#include <iostream>
#include <string.h>

#include <SDL3_net/SDL_net.h>

#include "types.h"
#include "player.h"
#include "other.h"
#include "net.h"
#include "server.h"

// TODO: add some kind of 'Broadcast' function that sends a package to all players

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
}

Server::Server(const char * levelname) {
	// TODO: Set levelname here?
}

Server::~Server() {
	// Todo: Belongs in some kind of collisionmap class
	for(int row = 0; row < lvl.width; row++)
   	{
     		delete collisionmap[row];
	}
	delete collisionmap;
	SDLNet_DestroyDatagramSocket(udpsock);
	SDLNet_Quit();
}

Uint8 Server::CheckVictory() {
	if( red_team.bases == 0 ) {
		std::cout << "BLUE WINS" << std::endl;
		return BLUE;
	}
	if( blue_team.bases == 0 ) {
		std::cout << "RED WINS" << std::endl;
		return RED;
	}
	return NEUTRAL;
}

void Server::LoadLevel() {
	if( !LoadLevelFile() ) // load everything into the lvl struct
	{	
		// Todo: Belongs in some kind of collisionmap class
		for(int row = 0; row < lvl.width; row++)
   		{
     			delete this->collisionmap[row];
		}
		delete this->collisionmap;
		error = 5;
		SDLNet_DestroyDatagramSocket(this->udpsock);
		SDLNet_Quit();
		this->runstate = SERVER_RUNSTATE_FAIL;
		return;
	}
}

void Server::Init() {

	this->runstate = SERVER_RUNSTATE_OK;
	//initiate SDL_NET
	if(!SDLNet_Init())
	{
 		printf("SDLNet_Init: %s\n", SDL_GetError());
		SDL_Quit();
		this->runstate = SERVER_RUNSTATE_FAIL;
		return;
	}

	// Listen on all available local addresses
	this->udpsock = SDLNet_CreateDatagramSocket(NULL, PORT_SERVER);
	if(!this->udpsock)
	{
    		printf("SDLNet_UDP_Open: %s\n", SDL_GetError());
			SDL_Quit();
			this->runstate = SERVER_RUNSTATE_FAIL;
			return;
	}

	this->LoadLevel();
	
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
}

// Run the server
void Server::Run() {
	this->Init();
	while( this->runstate == SERVER_RUNSTATE_OK ) {
		this->GameLoop();
	}
}

// Update timers
void Server::Tick() {
	oldtime = newtime;
	newtime = SDL_GetTicks();
	deltatime = newtime - oldtime;
}

void Server::HandleLeave() {
	// the players tells us he's leaving.
	// remove him from the player list and send notification to all the
	// other players.

	// TODO: validate whether leave number is in range of array

	Player *leaving_player = &players[in->buf[1] -1];

	std::cout << "@ player " << leaving_player->name << " left" << std::endl;
	number_of_players--;

	leaving_player->playing = 0;
	if( leaving_player->Team == BLUE )
	{
		blue_team.players--;
	}
	if( leaving_player->Team == RED )
	{
		red_team.players--;
	}
						
	// dereference player address
	SDLNet_UnrefAddress(leaving_player->playaddr);
	InitPlayer( leaving_player );
	// player has been removed
	// now notify all the other players
	for( int playerleaves = 0; playerleaves < MAXPLAYERS; playerleaves++ )
	{
		if( !players[ playerleaves ].playing )
		{
			continue;
		}

		sendbuf.Clear();
		sendbuf.Write8(PROTOCOL_PLAYER_LEAVES);
		sendbuf.Write8(playerleaves + 1); // Intended recipient
		sendbuf.Write8(in->buf[1]); // The leaving player

		SendBuffer(players[ playerleaves ].playaddr);
	}
				
}

// Send the sendbuffer to a client
void Server::SendBuffer(SDLNet_Address * client_address) {
	// TODO: Check for return value
	SDLNet_SendDatagram(udpsock,
						client_address,
						PORT_CLIENT, 
						(void *)sendbuf.data, 
						sendbuf.length); 
}

void Server::HandleUpdate() {
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

void Server::HandleJoin() {
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
				sendbuf.Clear();
				sendbuf.Write8(PROTOCOL_PLAYER_JOINS);
				sendbuf.Write8(sendplays + 1);
				sendbuf.Write8(newnum);
				sendbuf.Write8(newteam);
				sendbuf.WriteString(players[newnum-1].name);
				sendbuf.Write8('\0');
				SendBuffer(players[sendplays].playaddr);
			}
		}
		// now send the player himself a message with his name etc.
		//S: 030 PLAYER NAME P1 (NAME) P2 (NAME) P3 (NAME) P4 (NAME) P5
		//   (NAME) P6 (NAME) P7 (NAME) P8 (NAME) 
		sendbuf.Clear();
		sendbuf.Write8(PROTOCOL_JOIN);
		sendbuf.Write8(newnum);
		for( int gn = 0; gn < MAXPLAYERS; gn++ )
		{
			if( players[ gn ].playing )
			{
				sendbuf.Write16( (Uint16) players[ gn ].Team);
			}
			else
			{
				sendbuf.Write16( (Uint16) players[ gn ].playing);
			}
			sendbuf.WriteString(players[gn].name);
		}
		sendbuf.Write8('\0');
		SendBuffer(players[newnum-1].playaddr);
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

void Server::HandleStatus() {
	// we got a query, return server status/info
	// Protocol: S: 020 VERSION PLAYERS MAXPLAYERS TYPE LEVELCODE LEVELVERSION

	int templen = 0;
						
	sendbuf.Clear();
	int count = 0;
	
	sendbuf.Write8(PROTOCOL_STATUS);
	sendbuf.Write8(SHIPZ_VERSION);
	sendbuf.Write8(number_of_players);
	sendbuf.Write8(MAXPLAYERS);
	sendbuf.Write8(1); // TODO: Change later?
	sendbuf.Write8(1); // TODO: Change later?
	
	sendbuf.WriteString(lvl.filename);
	sendbuf.Write8('\0');
	
	SendBuffer(in->addr);
}

void Server::HandleChat() {
	int tempplay = in->buf[1];

	sendbuf.Clear();
	sendbuf.Write8(PROTOCOL_CHAT);
	sendbuf.Write8(0);
	sendbuf.WriteString((const char*)&in->buf[2]);
	sendbuf.Write8('\0');
	for( int sc = 0; sc < MAXPLAYERS; sc++ )
	{
		if( players[sc].playing && sc != (tempplay-1) )
		{
			sendbuf.SetPosByte(1, sc+1);
			SendBuffer(players[sc].playaddr);
		}
	}
}

void Server::CheckIdlePlayers() {
	// if a player is idle for 2 secs, kick him:
	for( int ci = 0; ci < MAXPLAYERS; ci++ )
	{
		if( (SDL_GetTicks() - players[ci].lastsendtime) > IDLETIMEBEFOREDROP && players[ci].playing )
		{
			// player is idle, kick him and send him kick notification
			// remove him from the player list and send notification to all the
			// other players.
			std::cout << "player " << ( ci + 1 ) << " idle > 2 sec. kicking. " << std::endl;
			// TODO: Figure out how this is supposed to work
			sendbuf.Clear();
			sendbuf.Write8(0);
			sendbuf.Write8(0);
			sendbuf.Write8(0);
			SendBuffer(players[ci].playaddr);
			
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
				if( !players[playerleaves].playing )
				{
					continue;
				}
				sendbuf.Clear();
				sendbuf.Write8(PROTOCOL_PLAYER_LEAVES);
				sendbuf.Write8(playerleaves + 1);
				sendbuf.Write8(ci + 1);
				SendBuffer(players[playerleaves].playaddr);
			}
		}
	}
}

// Update player physics, collisions, etc
void Server::UpdatePlayers() {
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
				// update etc.
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

}

void Server::SendUpdates() {
	// send all the stuff to all the players
	// S: 040 PLAYER BASESTATES TEAMSTATES (PSTAT PFRAME PX PY PVX PVY BULX BULY BULVX BULVY) x8
	sendbuf.Clear();
	sendbuf.Write8(PROTOCOL_UPDATE);
	sendbuf.Write8(0);

	Uint32 basestates = 0;
	for( int bidx = 0; bidx < MAXBASES; bidx++ ) {
		if( bases[bidx].owner == RED ) {
			basestates |= (1 << (bidx *2));
		}
		if( bases[bidx].owner == BLUE ) {
			basestates |= (1 << (bidx *2 +1));
		}
	}
	sendbuf.Write32( basestates);
	
	sendbuf.Write16( Sint16( red_team.bases ));
	sendbuf.Write16( Sint16( blue_team.bases ));
	
	for( int wp = 0; wp < MAXPLAYERS; wp++ )
	{
		sendbuf.Write16( Sint16( players[wp].status ));
		sendbuf.Write16( Sint16( players[wp].shipframe ));
		sendbuf.Write16( Sint16( players[wp].typing ));
		sendbuf.Write16( Sint16( players[wp].x ));
		sendbuf.Write16( Sint16( players[wp].y ));
		sendbuf.Write16( Sint16( players[wp].vx ));
		sendbuf.Write16( Sint16( players[wp].vy ));
	
		if( players[wp].bullet_shot )
		{
			sendbuf.Write16( Sint16( players[wp].bulletshotnr ));
			sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].type ));
			if( bullets[players[wp].bulletshotnr].type == BULLET ||
				bullets[players[wp].bulletshotnr].type == MINE )
			{
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].x ));
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].y ));
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].vx ));
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].vy ));
			}
			if( bullets[players[wp].bulletshotnr].type == ROCKET )
			{
				sendbuf.Write16( 0);
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].angle ));
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].x ));
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].y ));
			}
			players[wp].bullet_shot = 0;
		}
		else
		{
			sendbuf.Write16( 0 );
			sendbuf.Write16( 0 );
			sendbuf.Write16( 0 );
			sendbuf.Write16( 0 );
			sendbuf.Write16( 0 );
			sendbuf.Write16( 0 );
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

	sendbuf.Write16( Sint16( bulcount ));

	for( Sint16 wrb = 0; wrb < NUMBEROFBULLETS; wrb++ )
	{
		if( bullets[wrb].active == 1 && bullets[wrb].collide == 1 )
		{
			sendbuf.Write16( (Sint16)wrb);
			CleanBullet( int( wrb ));
		}
	}

	
	for( int sp = 0; sp < MAXPLAYERS; sp++ )
	{
		if( players[sp].playing )
		{
			sendbuf.SetPosByte(1, sp + 1);
			SendBuffer(players[sp].playaddr);
		
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

void Server::GameLoop() {
	while(done == 0)
	{
		if( CheckForQuitSignal() )
		{
			done = 1;
			this->runstate = SERVER_RUNSTATE_QUIT;
			std::cout << "@ quitting.." << std::endl;
		}
		this->Tick();
		
		if(SDLNet_ReceiveDatagram(udpsock, &in) && in != NULL ) {
			switch(in->buf[0]) {
				case PROTOCOL_LEAVE:
					this->HandleLeave();
					break;
				case PROTOCOL_UPDATE:
					this->HandleUpdate();
					break;
				case PROTOCOL_JOIN:
					this->HandleJoin();
					break;
				case PROTOCOL_STATUS:
					this->HandleStatus();
					break;
				case PROTOCOL_CHAT:
					this->HandleChat();
					break;
			}
			// Free datagram:
			SDLNet_DestroyDatagram(in);
		}

		this->CheckIdlePlayers();

		UpdateBullets( players );
		CheckBulletCollides( collisionmap );
		UpdatePlayers();
		// TODO: Update logic:
		// If a player conquers the last base, keep
		// running the base loop but do not allow further captures
		// After a few seconds, send event quit
		if(CheckVictory() != NEUTRAL) {
			auto event = new EventTeamWins(CheckVictory());
			SendEvent(event);
			done = true;
			runstate = SERVER_RUNSTATE_QUIT;

		} else if((float(SDL_GetTicks()) - lastsendtime) > SENDDELAY)
		{
			this->SendUpdates();
		}
	}
}

void Server::SendEvent(Event *event) {
	this->sendbuf.Clear();
	this->sendbuf.Write8(PROTOCOL_EVENT);
	if(!event->Serialize(&this->sendbuf)) {
		throw new std::runtime_error("Insufficient buffer for sending event");
	}
	for(uint p = 0; p < MAXPLAYERS; p++ ) {
		SendBuffer(players[p].playaddr);
	}
}
