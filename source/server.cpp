#include <iostream>
#include <string.h>

#include <SDL3_net/SDL_net.h>

#include "types.h"
#include "player.h"
#include "other.h"
#include "net.h"


void UpdateBases() {
	red_team.bases = 0;
	blue_team.bases = 0;
	for(int bidx =0; bidx < MAXBASES; bidx++ ) {
		if (bases[bidx].owner == RED) {
			red_team.bases++;
		}
		if (bases[bidx].owner == BLUE) {
			blue_team.bases++;
		}
	}
	if( red_team.bases == 0 ) {
		std::cout << "BLUE WINS" << std::endl;
	}
	if( blue_team.bases == 0 ) {
		std::cout << "RED WINS" << std::endl;
	}
}

int Server()
{
	Uint8 sendbuf[MAXBUFSIZE];
	SDLNet_Datagram * in;
	SDLNet_Address * ipaddr;
	SDLNet_DatagramSocket * udpsock;
	int error = 0;
	Player players[MAXPLAYERS];
	bool ** collisionmap;
	int done = 0;
	int number_of_players = 0;
	SDLNet_Address * my_ip_address;

	//initiate SDL_NET
	if(!SDLNet_Init())
	{
 		printf("SDLNet_Init: %s\n", SDL_GetError());
		SDL_Quit();
		exit(2);
	}

	// Listen on all available local addresses
	udpsock = SDLNet_CreateDatagramSocket(NULL, PORT_SERVER);
	if(!udpsock)
	{
    		printf("SDLNet_UDP_Open: %s\n", SDL_GetError());
			SDL_Quit();
    		exit(2);
	}
	

	// note to self: server should internally give a player status 'joining' so it knows the clients isn't just
	// being idle
	
	if( !LoadLevelFile() ) // load everything into the lvl struct
	{	
		for(int row = 0; row < lvl.width; row++)
   		{
     			delete collisionmap[row];
		}
		delete collisionmap;
		error = 5;
		SDLNet_DestroyDatagramSocket(udpsock);
		SDLNet_Quit();
		return error;
	}
	
	collisionmap = new bool *[lvl.width];
	
	for(int row = 0; row < lvl.width; row++)
   	{
     		collisionmap[row] = new bool[lvl.height];
	}

	GetCollisionMaps( collisionmap ); // load level & ship collisionmap
	
	for(int ep = 0; ep < MAXPLAYERS; ep++ )
	{
		EmptyPlayer( &players[ep] );
	}
	
	CreateGonLookup();
	for( int zb = 0; zb < NUMBEROFBULLETS; zb++ ) // clean all bullets before doing anything...
	{
		CleanBullet( zb );
	}
	
	red_team.frags = 0;
	blue_team.frags = 0;
	
	while(done == 0)
	{
		if( CheckForQuitSignal() )
		{
			done = 1;
		}
		oldtime = newtime;
		newtime = SDL_GetTicks();
		deltatime = newtime - oldtime;
		
		if(SDLNet_ReceiveDatagram(udpsock, &in) && in != NULL ) {
			Uint8 * tmpptr = in->buf;
			if( in->buf[0] == PROTOCOL_LEAVE )
			{
				// the players tells us he's leaving.
				// remove him from the player list and send notification to all the
				// other players.
				number_of_players--;
				players[in->buf[1] - 1].playing = 0;
				if( players[in->buf[1] - 1].Team == BLUE )
				{
					blue_team.players--;
				}
				if( players[in->buf[1] - 1].Team == RED )
				{
					red_team.players--;
				}
									
				// dereference player address
				SDLNet_UnrefAddress(players[in->buf[1]-1].playaddr);
				InitPlayer( &players[in->buf[1] - 1] );
				// player has been removed
				// now notify all the other players
				for( int playerleaves = 0; playerleaves < MAXPLAYERS; playerleaves++ )
				{
					memset( sendbuf, '\0', sizeof(sendbuf) );
					sendbuf[0] = PROTOCOL_PLAYER_LEAVES;
					sendbuf[1] = playerleaves + 1;
					sendbuf[2] = in->buf[1];
					if( players[ playerleaves ].playing )
					{
						// TODO: Check for return value
						SDLNet_SendDatagram(udpsock, players[ playerleaves ].playaddr, PORT_CLIENT, sendbuf, 3);
					}
				}
				
			}
			if( in->buf[0] == PROTOCOL_UPDATE )
			{
				int playerread = in->buf[1];
				playerread--;
				// client let's us know how he's doing, how nice of him.
				// read his status, buls, etc.
				
				Uint8 * tmpptr = in->buf;
			
				if( in->buf[2] == SUICIDE )
				{
					players[playerread].status = DEAD;
				}
				if( in->buf[2] == LIFTOFF )
				{
					std::cout << players[playerread].name << " wants to liftoff" << std::endl;
					if( players[playerread].status == LANDED )
					{
						std::cout << players[playerread].name << " taking off from ground" << std::endl;
						// should check for 3 sec shooting delay!!
						players[playerread].status = FLYING;
					}
					if( players[playerread].status == LANDEDBASE )
					{
						std::cout << players[playerread].name << " taking off from Base" << std::endl;
						// should check for 3 sec shooting delay!!
						players[playerread].status = FLYING;
					}
					if( players[playerread].status == LANDEDRESPAWN )
					{
						std::cout << players[playerread].name << " taking off from spawn" << std::endl;
						players[playerread].status = FLYING;
					}
				}
				if( players[playerread].status == DEAD )
				{
					if( in->buf[2] == RESPAWN )
					{
						std::cout << players[playerread].name << " wants to respawn" << std::endl;
						players[playerread].x = -14;       // this is needed for the
						players[playerread].y = -14;       // ship to appear correctly
						players[playerread].shipframe = 0; // with all the clients
						players[playerread].status = LANDEDRESPAWN;
					}
				}
				tmpptr+=3;
				players[playerread].shipframe = (Sint16)Read16( tmpptr );
				tmpptr+=2;
				players[playerread].typing = (Sint16)Read16( tmpptr );
				tmpptr+=2;					
				players[playerread].x = (Sint16)Read16( tmpptr );
				tmpptr+=2;
				players[playerread].y = (Sint16)Read16( tmpptr );
				tmpptr+=2;
				players[playerread].vx = (Sint16)Read16( tmpptr );
				tmpptr+=2;
				players[playerread].vy = (Sint16)Read16( tmpptr );
				tmpptr+=2;
				Sint16 tx = 0, ty = 0, tvx = 0, tvy = 0, tn =0, tbultyp=0;
				
				tn = (Sint16)Read16( tmpptr );
				tmpptr+=2;
				tbultyp = (Sint16)Read16( tmpptr );
				tmpptr+=2;
				tx = (Sint16)Read16( tmpptr );
				tmpptr+=2;
				ty = (Sint16)Read16( tmpptr );
				tmpptr+=2;
				tvx = (Sint16)Read16( tmpptr );
				tmpptr+=2;
				tvy = (Sint16)Read16( tmpptr );
				tmpptr+=2;
				
				if( tx == 0 && ty == 0 && tvx == 0 && tvy == 0 && tn == 0 && tbultyp == 0 )
				{
					// no bullet shot
					players[playerread].bullet_shot = 0;
				}
				else
				{
					players[playerread].bullet_shot = 1;
					players[playerread].bulletshotnr = Uint16(tn);
					players[playerread].lastshottime = SDL_GetTicks();

					if( tbultyp == BULLET )
					{
						bullets[tn].x = (float)tx;
						bullets[tn].y = (float)ty;
						bullets[tn].vx = (float)tvx;
						bullets[tn].vy = (float)tvy;
					}
					if( tbultyp == ROCKET )
					{
						// this may look wrong, but it's correct, trust me
						bullets[tn].x = (float)tvx;
						bullets[tn].y = (float)tvy;
						bullets[tn].angle = (float)ty;
					}
					if( tbultyp == MINE )
					{
						bullets[tn].x = (float)tx;
						bullets[tn].y = (float)ty;
						bullets[tn].minelaidtime = SDL_GetTicks();
					}
					
					bullets[tn].type = tbultyp;
					bullets[tn].owner = playerread + 1;
					bullets[tn].active = 1;
					bullets[tn].collide = 0;
				}
				
				
				players[ playerread ].lastsendtime = SDL_GetTicks();
			}
			if( in->buf[0] == PROTOCOL_JOIN )
			{
				// a player wants to join
				number_of_players++; 
				int newnum = 999; // this will hold the new player number if theres an empty spot.
				int newteam = 0;
				for( int searchempty = 1; searchempty < (MAXPLAYERS+1); searchempty++ )
				{
					if( !players[searchempty-1].playing )
					{
						newnum = searchempty;
						// we found a number, now find this (wo)man a Team!
						if( red_team.players <= blue_team.players )
						{
							newteam = RED;
							red_team.players++;
						}
						else
						{
							newteam = BLUE;
							blue_team.players++;
						}
						break;
					}
				}
				if( newnum != 999 )
				{
					strncpy( players[ newnum -1 ].name, 
								(const char * )&in->buf[1], 12 ); 
					players[ newnum - 1 ].playaddr = in->addr;
					// Increase ref count to this player's address
					SDLNet_RefAddress(in->addr);
					players[ newnum - 1 ].playing = 1;
					players[ newnum - 1 ].self_sustaining = 1;
					InitPlayer( &players[ newnum - 1 ] );
					players[ newnum - 1 ].Team = newteam;

					std::cout << "Player " << players[newnum-1].name
						<< " joined into slot " << newnum << std::endl;
						
					// issue a message to all players this player has joined:
					for( int sendplays = 0; sendplays < MAXPLAYERS; sendplays++ )
					{
						
						// 070 PLAYER PLAYJOINNR NAME
						if( players[ sendplays ].playing == 1 && sendplays != (newnum-1) )
						{
							memset( sendbuf, '\0', sizeof(sendbuf) );
							sendbuf[0] = PROTOCOL_PLAYER_JOINS;
							sendbuf[1] = sendplays + 1;
							sendbuf[2] = newnum;
							sendbuf[3] = newteam;
							strncpy( ( char *)&sendbuf[4],
									players[newnum-1].name, 12 ); 
							sendbuf[16] = '\0';

							// TODO: Check for return value
							SDLNet_SendDatagram(udpsock, players[sendplays].playaddr, PORT_CLIENT, sendbuf, 16);
						}
					}
					// now send the player himself a message with his name etc.
					//S: 030 PLAYER NAME P1 (NAME) P2 (NAME) P3 (NAME) P4 (NAME) P5
					//   (NAME) P6 (NAME) P7 (NAME) P8 (NAME) 
					int count = 0;
					memset( sendbuf, '\0', sizeof(sendbuf) );
					sendbuf[0] = PROTOCOL_JOIN;
					sendbuf[1] = newnum; 
					count = 2;
					Uint8 * tmpptr;
					tmpptr = &sendbuf[2];
					for( int gn = 0; gn < MAXPLAYERS; gn++ )
					{
						if( players[ gn ].playing )
						{
							Write16( (Uint16) players[ gn ].Team, tmpptr );
						}
						else
						{
							Write16( (Uint16) players[ gn ].playing, tmpptr );
						}
						tmpptr+=2;
						count+=2;
						strncpy( (char*)tmpptr,
							(const char *)players[ gn ].name, 12 ); 
						tmpptr+=12;
						count+=12;
					}
					*tmpptr = '\0';
					
					// TODO: Check returnvalue
					SDLNet_SendDatagram(udpsock, players[newnum-1].playaddr, PORT_CLIENT, sendbuf, count);

					// update players's lastsendtime:
					players[ newnum -1 ].lastsendtime = SDL_GetTicks();
				}
				else
				{
					// server is full, sent nothing, client will figure out on his own.
					// ( rarely happens because client checks this before joining )
					// should fix this in a later stage though..;
				}
			}
			if( in->buf[0] == PROTOCOL_STATUS )
			{
				// we got a query, return server status/info
				// Protocol: S: 020 VERSION PLAYERS MAXPLAYERS TYPE LEVELCODE LEVELVERSION

				int templen = 0;
									
				memset( sendbuf, '\0', sizeof(sendbuf) );
				int count = 0;
				
				sendbuf[0] = PROTOCOL_STATUS;
				sendbuf[1] = SHIPZ_VERSION;
				sendbuf[2] = number_of_players;
				sendbuf[3] = MAXPLAYERS;
				sendbuf[4] = 1; // change later
				sendbuf[5] = 1; // change later
				count = 6;
				
				snprintf( (char*)&sendbuf[6], MAXBUFSIZE-7, "%s", lvl.filename );					
				sendbuf[strlen( lvl.filename ) + 6 ] = '\0';
				count += strlen( lvl.filename );
				
				// send the package...
				SDLNet_SendDatagram(udpsock, in->addr, PORT_CLIENT, sendbuf, count);
			}
			// If we receive a chat package forward it to all players
			if( in->buf[0] == PROTOCOL_CHAT )
			{
				int tempplay = in->buf[1];
				memset( sendbuf, '\0', sizeof( sendbuf ));
				sendbuf[0] = PROTOCOL_CHAT;
				strncpy( (char *)&sendbuf[2], (const char*)&in->buf[2], in->buflen - 2 );
				for( int sc = 0; sc < MAXPLAYERS; sc++ )
				{
					if( players[sc].playing && sc != (tempplay-1) )
					{
						sendbuf[1] = sc+1;
						SDLNet_SendDatagram(udpsock, players[sc].playaddr, PORT_CLIENT, sendbuf, in->buflen);
					}
				}
				// this is a chat package, do this shit later....
			}
			// Free datagram:
			SDLNet_DestroyDatagram(in);
		}

		// if a player is idle for 2 secs, kick him:
		for( int ci = 0; ci < MAXPLAYERS; ci++ )
		{
			if( (SDL_GetTicks() - players[ci].lastsendtime) > IDLETIMEBEFOREDROP && players[ci].playing )
			{
				// player is idle, kick him and send him kick notification
				// remove him from the player list and send notification to all the
				// other players.
				std::cout << "player " << ( ci + 1 ) << " idle > 2 sec. kicking. " << std::endl;
				memset( sendbuf, '\0', sizeof(sendbuf) );
				
				SDLNet_SendDatagram(udpsock, players[ci].playaddr, PORT_CLIENT, sendbuf, 3);
				
				number_of_players--;
				if( players[ci].Team == BLUE )
				{
					blue_team.players--;
				}
				if( players[ci].Team == RED )
				{
					red_team.players--;
				}

				players[ ci ].playing = 0;
				// Dereference address
				SDLNet_UnrefAddress(players[ci].playaddr);
				InitPlayer( &players[ ci ] );
				// player has been removed / kicked
				// now notify all the other players
				for( int playerleaves = 0; playerleaves < MAXPLAYERS; playerleaves++ )
				{
					memset( sendbuf, '\0', sizeof(sendbuf) );
					sendbuf[0] = PROTOCOL_PLAYER_LEAVES;
					sendbuf[1] = playerleaves + 1;
					sendbuf[2] = ci + 1;
					if( players[playerleaves].playing )
					{
						SDLNet_SendDatagram(udpsock, players[playerleaves].playaddr, PORT_CLIENT, sendbuf, 3);
					}
				}
			}
		}

		UpdateBullets( players );
		CheckBulletCollides( collisionmap );
		for( int up = 0; up < MAXPLAYERS; up++ )
		{
			if( players[up].playing && players[up].status == FLYING )
			{
				UpdatePlayer( &players[up] );
				
				int baseresult;
				baseresult = PlayerCollideWithBase( &players[up] );
				if( baseresult != -1 )
				{
					if( players[up].vx < 40 && players[up].vx > -40 &&
					    players[up].vy < 60 && players[up].vy > 0 )
					{
						players[up].status = LANDEDBASE;
						if( bases[ baseresult ].owner != players[up].Team )
						{
							std::cout << "team " << players[up].Team << " has captured base #" << baseresult << std::endl;
							bases[ baseresult ].owner = players[up].Team;
							UpdateBases();
						}
					}
					else
					{
						if( players[up].Team == RED )
						{
							red_team.frags--;
						}
						else
						{
							blue_team.frags--;
						}
						players[up].status = JUSTCOLLIDEDBASE; // JUSTCOLLIDEDBASE
					}
				}
			
				if( PlayerCollideWithLevel( &players[up], collisionmap ))
				{
					if( players[up].vx < 40 && players[up].vx > -40 &&
					    players[up].vy < 60 && players[up].vy > 0)
					{
						players[up].status = LANDED;
					}
					else
					{
						// send a chat pkg! :P
						if( players[up].Team == RED )
						{
							red_team.frags--;
						}
						else
						{
							blue_team.frags--;
						}
						std::cout << "player " << players[up].name << "collided with rock at" << players[up].x << "," << players[up].y << std::endl;
						players[up].status = JUSTCOLLIDEDROCK;
					}
				}
				int bulletresult = PlayerCollideWithBullet( &players[up], up+1, players );
				if( bulletresult != -1 )
				{
					// the function returns -1 when a player didn't collide, so he must've collided
					// note down the bullet for removal and change the player status
					// NOTE: in a later stage we should report which player shot him and
					// update money, etc.
					if( bullets[bulletresult].owner == RED )
					{
						if( bullets[bulletresult].type == MINE &&
							players[up].Team == RED )
						{
							red_team.frags--;
						}
						else
						{
							red_team.frags++;
						}
					}
					else
					{
						if( bullets[bulletresult].type == MINE &&
							players[up].Team == BLUE )
						{
							blue_team.frags--;
						}
						else
						{
							blue_team.frags++;
						}
					}

					bullets[bulletresult].collide = 1;
					
					players[up].status = JUSTSHOT;
				}
			}
		}
						
		if((float(SDL_GetTicks()) - lastsendtime) > SENDDELAY)
		{
			// send all the stuff to all the players
			// S: 040 PLAYER BASESTATES TEAMSTATES (PSTAT PFRAME PX PY PVX PVY BULX BULY BULVX BULVY) x8
			memset( sendbuf, '\0', sizeof(sendbuf) );
			sendbuf[0] = PROTOCOL_UPDATE;
			sendbuf[1] = 0;
			int count = 2;
		
			Uint8 * tmpptr = sendbuf;
			tmpptr+=2;

			Uint32 basestates = 0;
			for( int bidx = 0; bidx < MAXBASES; bidx++ ) {
				if( bases[bidx].owner == RED ) {
					basestates |= (1 << (bidx *2));
				}
				if( bases[bidx].owner == BLUE ) {
					basestates |= (1 << (bidx *2 +1));
				}
			}
			Write32( basestates, tmpptr);
			tmpptr+=4;
			
			Write16( Sint16( red_team.bases ), tmpptr );
			tmpptr+=2;
			Write16( Sint16( blue_team.bases ), tmpptr );
			tmpptr+=2;
			count +=4;
			
			for( int wp = 0; wp < MAXPLAYERS; wp++ )
			{
				Write16( Sint16( players[wp].status ), tmpptr );
				tmpptr+=2;
				Write16( Sint16( players[wp].shipframe ), tmpptr );
				tmpptr+=2;
				Write16( Sint16( players[wp].typing ), tmpptr );
				tmpptr+=2;
				Write16( Sint16( players[wp].x ), tmpptr );
				tmpptr+=2;
				Write16( Sint16( players[wp].y ), tmpptr );
				tmpptr+=2;
				Write16( Sint16( players[wp].vx ), tmpptr );
				tmpptr+=2;
				Write16( Sint16( players[wp].vy ), tmpptr );
				tmpptr+=2;
				count +=14;
			
				if( players[wp].bullet_shot )
				{
					Write16( Sint16( players[wp].bulletshotnr ), tmpptr );
					tmpptr+=2;
					Write16( Sint16( bullets[players[wp].bulletshotnr].type ), tmpptr );
					tmpptr+=2;
					if( bullets[players[wp].bulletshotnr].type == BULLET ||
						bullets[players[wp].bulletshotnr].type == MINE )
					{
						Write16( Sint16( bullets[players[wp].bulletshotnr].x ), tmpptr );
						tmpptr+=2;
						Write16( Sint16( bullets[players[wp].bulletshotnr].y ), tmpptr );
						tmpptr+=2;
						Write16( Sint16( bullets[players[wp].bulletshotnr].vx ), tmpptr );
						tmpptr+=2;
						Write16( Sint16( bullets[players[wp].bulletshotnr].vy ), tmpptr );
						tmpptr+=2;
					}
					if( bullets[players[wp].bulletshotnr].type == ROCKET )
					{
						Write16( 0, tmpptr );
						tmpptr+=2;
						Write16( Sint16( bullets[players[wp].bulletshotnr].angle )
									, tmpptr );
						tmpptr+=2;
						Write16( Sint16( bullets[players[wp].bulletshotnr].x ), tmpptr );
						tmpptr+=2;
						Write16( Sint16( bullets[players[wp].bulletshotnr].y ), tmpptr );
						tmpptr+=2;

					}
					count+=12;
					players[wp].bullet_shot = 0;
				}
				else
				{
					Write16( 0, tmpptr );
					tmpptr+=2;
					Write16( 0, tmpptr );
					tmpptr+=2;
					Write16( 0, tmpptr );
					tmpptr+=2;
					Write16( 0, tmpptr );
					tmpptr+=2;
					Write16( 0, tmpptr );
					tmpptr+=2;
					Write16( 0, tmpptr );
					tmpptr+=2;
					count+=12;
					players[wp].bullet_shot = 0;

				}
			}
			
			Sint16 bulcount = 0;
			for( int cnt = 0; cnt < NUMBEROFBULLETS; cnt++ )
			{
				if( bullets[cnt].active == 1 && bullets[cnt].collide == 1 )
				{
					bulcount++;
				}
			}

			Write16( Sint16( bulcount ), tmpptr );
			tmpptr+=2;
			count +=2;

			for( Sint16 wrb = 0; wrb < NUMBEROFBULLETS; wrb++ )
			{
				if( bullets[wrb].active == 1 && bullets[wrb].collide == 1 )
				{
					Write16( (Sint16)wrb, tmpptr );
					tmpptr+=2;
					count +=2;
					CleanBullet( int( wrb ));
				}
			}

			
			for( int sp = 0; sp < MAXPLAYERS; sp++ )
			{
				if( players[sp].playing )
				{
					sendbuf[1] = ( sp + 1 );
					SDLNet_SendDatagram(udpsock, players[sp].playaddr, PORT_CLIENT, sendbuf, count);
				
					// deal with stati
					if(players[sp].status == JUSTCOLLIDEDBASE)
					{
						players[sp].status = DEAD;
					}
					if(players[sp].status == JUSTCOLLIDEDROCK)
					{
						players[sp].status = DEAD;
					}
					if( players[sp].status == JUSTSHOT )
					{
						players[sp].status = DEAD;
					}
				}
			}
			lastsendtime = float(SDL_GetTicks());
		}
		
	}

	for(int row = 0; row < lvl.width; row++)
   	{
     		delete collisionmap[row];
	}
	delete collisionmap;
	SDLNet_DestroyDatagramSocket(udpsock);
	SDLNet_Quit();
	return 0;
}
