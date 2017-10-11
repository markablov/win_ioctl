{
    'targets':
    [
        {
            'target_name': 'win_ioctl',
            'sources': ['src/main.cc' ],
            'include_dirs':
	        [
                '<!(node -e "require(\'nan\')")'
            ]
        }
    ]
}
