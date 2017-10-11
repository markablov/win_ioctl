#include <WinSock2.h>
#include <windows.h>
#include <io.h>
#include <nan.h>

using namespace v8;
using namespace node;

// node.js openSync/open always calls CreateFile without FILE_FLAG_OVERLAPPED, so we can't use 
// built-in async way for calling WinAPI, and should use libuv

class IoctlCaller
{
private:
	std::string m_err;
	int m_errno;
	void * m_out;
	unsigned long m_outLen;
public:
	bool isSuccess()
	{
		return m_err.empty();
	}

	const char * errorMsg()
	{
		return m_err.c_str();
	}

	int errorCode()
	{
		return m_errno;
	}

	const void * out()
	{
		return m_out;
	}

	unsigned long outLength()
	{
		return m_outLen;
	}

	IoctlCaller() : m_out(nullptr), m_outLen(0), m_errno(-1), m_err("Operation have not been performed") {}

	IoctlCaller(HANDLE handle, unsigned int code, void * buffer, unsigned int bufferSize, unsigned int outBufferSize) : m_errno(-1), m_outLen(0), m_out(nullptr)
	{
		if (outBufferSize)
		{
			m_out = malloc(outBufferSize);
			if (!m_out)
			{
				m_errno = ENOMEM;
				m_err = "Could not allocate memory for output buffer";
				return;
			}
		}
		if (!DeviceIoControl(handle, code, buffer, bufferSize, m_out, outBufferSize, &m_outLen, nullptr))
		{
			if (m_out)
				free(m_out);
			unsigned int err = GetLastError();
			if (err == ERROR_INSUFFICIENT_BUFFER || err == ERROR_MORE_DATA)
			{
				m_errno = ERANGE;
				m_err = "Output buffer is too small to handle result";
			}
			else
			{
				m_errno = -1;
				m_err = "Unknown error " + std::to_string(err);
			}
		}
	}

	~IoctlCaller() {}
};

class IoctlWorker: public Nan::AsyncWorker
{
public:
	IoctlWorker(Nan::Callback *callback, HANDLE handle, unsigned int code, void * buffer, unsigned int bufferSize, unsigned int outBufferSize)
		: Nan::AsyncWorker(callback), m_handle(handle), m_code(code), m_buffer(buffer), m_bufferSize(bufferSize), m_outBufferSize(outBufferSize) {}

	~IoctlWorker() {}

	void Execute()
	{
		m_caller = IoctlCaller(m_handle, m_code, m_buffer, m_bufferSize, m_outBufferSize);
		if (!m_caller.isSuccess())
			this->SetErrorMessage(m_caller.errorMsg());
	}

	void HandleOKCallback()
	{
		Nan::HandleScope scope;

		Local<Value> argv[] =
		{
			Nan::Null(),
			Nan::NewBuffer((char *)m_caller.out(), m_caller.outLength()).ToLocalChecked()
		};

		callback->Call(2, argv);
	}

	void HandleErrorCallback()
	{
		Nan::HandleScope scope;

		int err = m_caller.errorCode();
		Local<Value> argv[] = { err > 0 ? Nan::ErrnoException(err, "win_ioctl", m_caller.errorMsg(), nullptr) : Nan::Error(m_caller.errorMsg()) };
		callback->Call(1, argv);
	}

private:
	HANDLE m_handle;
	unsigned int m_code;
	void * m_buffer;
	unsigned int m_bufferSize;
	unsigned int m_outBufferSize;
	IoctlCaller m_caller;
};

void invalid_parameter_handler(const wchar_t * expression, const wchar_t * function, const wchar_t * file, unsigned int line, uintptr_t pReserved){}

NAN_METHOD(win_ioctl)
{
	Nan::HandleScope scope;

	int argc = info.Length();
	Nan::Callback *callback = nullptr;

	if (argc < 4)
		return Nan::ThrowError("Insufficiently arguments.");

	if (argc > 4)
	{
		if (!info[4]->IsFunction())
			return Nan::ThrowTypeError("Argument 4 should be a function");
		callback = new Nan::Callback(info[4].As<Function>());
	}

#define RETURN_ERROR(err)\
{\
	if (callback)\
	{\
		Local<Value> argv[] = {Nan::Error(err)};\
		callback->Call(1, argv);\
		return;\
	} else\
	{\
		Nan::ThrowError(err);\
		return;\
	}\
}\

	if (!info[0]->IsInt32())
		RETURN_ERROR("Argument 0 should be a file descriptor");
	int fd = info[0]->Int32Value();
	_set_invalid_parameter_handler(invalid_parameter_handler);
	// use uv_get_osfhandle instead of _get_osfhandle coz of https://msdn.microsoft.com/en-us/library/ms235460.aspx
	HANDLE handle = uv_get_osfhandle(fd);
	if (handle < 0)
		RETURN_ERROR("Argument 0 should be a file descriptor");

	if (!info[1]->IsUint32())
		RETURN_ERROR("Argument 1 should be an integer");
	unsigned long code = info[1]->Uint32Value();

	void * buffer = nullptr;
	size_t bufferSize = 0;
	if (!info[2]->IsUndefined())
	{
		Local<Object> buf_obj = info[2]->ToObject();
		if (!Buffer::HasInstance(buf_obj))
			RETURN_ERROR("Argument 2 should be an instance of Buffer or undefined");
		buffer = Buffer::Data(buf_obj);
		bufferSize = Buffer::Length(buf_obj);
	}

	unsigned int outBufferSize = 0;
	if (!info[3]->IsUndefined())
	{
		if (!info[3]->IsUint32())
			RETURN_ERROR("Argument 3 should be an integer or undefined");
		outBufferSize = info[3]->Uint32Value();
	}

	if (callback)
	{
		Nan::AsyncQueueWorker(new IoctlWorker(callback, handle, code, buffer, bufferSize, outBufferSize));
	}
	else
	{
		IoctlCaller caller(handle, code, buffer, bufferSize, outBufferSize);
		if (caller.isSuccess())
			info.GetReturnValue().Set(Nan::NewBuffer((char *)caller.out(), caller.outLength()).ToLocalChecked());
		else
		{
			int err = caller.errorCode();
			return Nan::ThrowError(err > 0 ? Nan::ErrnoException(err, "win_ioctl", caller.errorMsg(), nullptr) : Nan::Error(caller.errorMsg()));
		}
	}
}

void init(Local<Object> exports, Local<Object> module)
{
	// win_ioctl(int fd, uint code, Buffer input, int outBufSize, [Function cb])
	module->Set(Nan::New("exports").ToLocalChecked(), Nan::New<FunctionTemplate>(win_ioctl)->GetFunction());
}

NODE_MODULE(win_ioctl, init)
