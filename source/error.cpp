#include <iostream>

#include "types.h"

void ShowError( int error )
{
	if( error != 0 )
	{
		switch( error )
		{
			case 1:
				std::cout << "Protocol error." << std::endl;
				std::cout << "This is most likely a bug. Waiting 10 seconds and then reconnecting" << std::endl;
				std::cout << "probably helps. If you keep having the problem," << std::endl;
				std::cout << "contact a Shipz developer please." << std::endl;
				break;
			case 2:
				std::cout << "Version mismatch." << std::endl;
				break;
			case 3:
				std::cout << "Server full." << std::endl;
				break;
			case 4:
				std::cout << "Server timed out ( 5 sec ). Trying a faster one might help." << std::endl;
				break;
			case 5:
				std::cout << "Couldn't load map file " << lvl.filename << std::endl;
				break;
			case 6:
				std::cout << "You were kicked." << std::endl;
				break;
			case 10: 
				std::cout << std::endl;
				std::cout << "USAGE:" << std::endl
				     << "client:" << std::endl
				     << "	$ shipz --client hostip name" << std::endl
				     << "server:" << std::endl
				     << "	$ shipz --server levelname" << std::endl << std::endl;
				break;
		}
	}
}
