{
    'targets':
	[
        {
            'target_name': 'win-ioctl',
            'sources': ['src/main.cc' ],
            'include_dirs':
			[
                '<!(node -e "require(\'nan\')")'
            ]
        }
    ]
}
