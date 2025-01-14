#include <iostream>
#include <string.h>

#include <SDL3_net/SDL_net.h>

#include "types.h"
#include "player.h"
#include "other.h"
#include "net.h"
#include "server.h"
#include "team.h"
#include "base.h"
#include "level.h"

// TODO: add some kind of 'Broadcast' function that sends a package to all players

void Server::UpdateBases() {
	red_team.bases = 0;
	blue_team.bases = 0;
	for(int bidx =0; bidx < MAXBASES; bidx++ ) {
		if (bases[bidx].owner == SHIPZ_TEAM::RED) {
			red_team.bases++;
		}
		if (bases[bidx].owner == SHIPZ_TEAM::BLUE) {
			blue_team.bases++;
		}
	}
}

Server::Server(const char * levelname) {
	lvl.SetFile(levelname);
}

Server::~Server() {
	// Todo: Belongs in some kind of collisionmap class
	for(int row = 0; row < lvl.m_width; row++)
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
		return SHIPZ_TEAM::BLUE;
	}
	if( blue_team.bases == 0 ) {
		std::cout << "RED WINS" << std::endl;
		return SHIPZ_TEAM::RED;
	}
	return SHIPZ_TEAM::NEUTRAL;
}

void Server::LoadLevel() {
	if( !lvl.Load() ) // load everything into the lvl struct
	{	
		// Todo: Belongs in some kind of collisionmap class
		for(int row = 0; row < lvl.m_width; row++)
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
	
	collisionmap = new bool *[lvl.m_width];
	
	for(int row = 0; row < lvl.m_width; row++)
   	{
     		collisionmap[row] = new bool[lvl.m_height];
	}

	GetCollisionMaps( collisionmap ); // load level & ship collisionmap
	
	for(int ep = 0; ep < MAXPLAYERS; ep++ )
	{
		players[ep].Empty();
	}
	
	CreateGonLookup();
	for( int zb = 0; zb < NUMBEROFBULLETS; zb++ ) // clean all bullets before doing anything...
	{
		CleanBullet( zb );
	}
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
	if( leaving_player->Team == SHIPZ_TEAM::BLUE )
	{
		blue_team.players--;
	}
	if( leaving_player->Team == SHIPZ_TEAM::RED )
	{
		red_team.players--;
	}
						
	// dereference player address
	SDLNet_UnrefAddress(leaving_player->playaddr);
	leaving_player->Empty();
	// player has been removed
	// now notify all the other players
	for( int playerleaves = 0; playerleaves < MAXPLAYERS; playerleaves++ )
	{
		if( !players[ playerleaves ].playing )
		{
			continue;
		}

		sendbuf.Clear();
		sendbuf.Write8(SHIPZ_MESSAGE::MSG_PLAYER_LEAVES);
		sendbuf.Write8(playerleaves + 1); // Intended recipient
		sendbuf.Write8(in->buf[1]); // The leaving player

		SendBuffer(players[ playerleaves ].playaddr);
	}
				
}

// Send the sendbuffer to a client
void Server::SendBuffer(SDLNet_Address * client_address) {
	std::cout << "Sending buffer: ";
	PrintRawBytes(this->sendbuf.AsString(), this->sendbuf.length);
	std::cout << std::endl;;
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

	if( in->buf[2] == PLAYER_STATUS::SUICIDE )
	{
		players[playerread].status = PLAYER_STATUS::DEAD;
	}
	if( in->buf[2] == PLAYER_STATUS::LIFTOFF )
	{
		std::cout << players[playerread].name << " wants to liftoff" << std::endl;
		if( players[playerread].status == PLAYER_STATUS::LANDED )
		{
			std::cout << players[playerread].name << " taking off from ground" << std::endl;
			// should check for 3 sec shooting delay!!
			players[playerread].status = PLAYER_STATUS::FLYING;
		}
		if( players[playerread].status == PLAYER_STATUS::LANDEDBASE )
		{
			std::cout << players[playerread].name << " taking off from Base" << std::endl;
			// should check for 3 sec shooting delay!!
			players[playerread].status = PLAYER_STATUS::FLYING;
		}
		if( players[playerread].status == PLAYER_STATUS::LANDEDRESPAWN )
		{
			std::cout << players[playerread].name << " taking off from spawn" << std::endl;
			players[playerread].status = PLAYER_STATUS::FLYING;
		}
	}
	if( players[playerread].status == PLAYER_STATUS::DEAD )
	{
		if( in->buf[2] == PLAYER_STATUS::RESPAWN )
		{
			std::cout << players[playerread].name << " wants to respawn" << std::endl;
			players[playerread].x = -14;       // this is needed for the
			players[playerread].y = -14;       // ship to appear correctly
			players[playerread].shipframe = 0; // with all the clients
			players[playerread].status = PLAYER_STATUS::LANDEDRESPAWN;
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

		if( tbultyp == WEAPON_BULLET )
		{
			bullets[tn].x = (float)tx;
			bullets[tn].y = (float)ty;
			bullets[tn].vx = (float)tvx;
			bullets[tn].vy = (float)tvy;
		}
		if( tbultyp == WEAPON_ROCKET )
		{
			// this may look wrong, but it's correct, trust me
			bullets[tn].x = (float)tvx;
			bullets[tn].y = (float)tvy;
			bullets[tn].angle = (float)ty;
		}
		if( tbultyp == WEAPON_MINE )
		{
			bullets[tn].x = (float)tx;
			bullets[tn].y = (float)ty;
			bullets[tn].minelaidtime = SDL_GetTicks();
		}
		
		bullets[tn].type = tbultyp;
		bullets[tn].owner = playerread + 1;
		bullets[tn].active = true;
		bullets[tn].collide = false;
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
				newteam = SHIPZ_TEAM::RED;
				red_team.players++;
			}
			else
			{
				newteam = SHIPZ_TEAM::BLUE;
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
		players[ newnum - 1 ].Init();
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
				sendbuf.Write8(SHIPZ_MESSAGE::MSG_PLAYER_JOINS);
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
		sendbuf.Write8(SHIPZ_MESSAGE::JOIN);
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
	// Protocol: S: 020 VERSION PLAYERS MAXPLAYERS TYPE 

	int templen = 0;
						
	sendbuf.Clear();
	int count = 0;
	
	sendbuf.Write8(SHIPZ_MESSAGE::STATUS);
	sendbuf.Write8(SHIPZ_VERSION);
	sendbuf.Write8(number_of_players);
	sendbuf.Write8(MAXPLAYERS);
	sendbuf.Write8(lvl.m_levelversion);
	sendbuf.WriteString(lvl.m_filename.c_str());

	sendbuf.Write8('\0');
	
	SendBuffer(in->addr);
}

void Server::HandleChat() {
	int tempplay = in->buf[1];

	sendbuf.Clear();
	sendbuf.Write8(SHIPZ_MESSAGE::CHAT);
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
			if( players[ci].Team == SHIPZ_TEAM::BLUE )
			{
				blue_team.players--;
			}
			if( players[ci].Team == SHIPZ_TEAM::RED )
			{
				red_team.players--;
			}

			players[ ci ].playing = 0;
			// Dereference address
			SDLNet_UnrefAddress(players[ci].playaddr);
			players[ ci ].Init();
			// player has been removed / kicked
			// now notify all the other players
			for( int playerleaves = 0; playerleaves < MAXPLAYERS; playerleaves++ )
			{
				if( !players[playerleaves].playing )
				{
					continue;
				}
				sendbuf.Clear();
				sendbuf.Write8(SHIPZ_MESSAGE::MSG_PLAYER_LEAVES);
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
		if( players[up].playing && players[up].status == PLAYER_STATUS::FLYING )
		{
			players[up].Update();
			
			int baseresult;
			baseresult = PlayerCollideWithBase( &players[up] );
			if( baseresult != -1 )
			{
				if( players[up].vx < 40 && players[up].vx > -40 &&
					players[up].vy < 60 && players[up].vy > 0 )
				{
					players[up].status = PLAYER_STATUS::LANDEDBASE;
					if( bases[ baseresult ].owner != players[up].Team )
					{
						std::cout << "team " << players[up].Team << " has captured base #" << baseresult << std::endl;
						bases[ baseresult ].owner = players[up].Team;
						UpdateBases();
					}
				}
				else
				{
					if( players[up].Team == SHIPZ_TEAM::RED )
					{
						red_team.frags--;
					}
					else
					{
						blue_team.frags--;
					}
					std::cout << "player " << players[up].name << " collided with base at " << players[up].x << "," << players[up].y << std::endl;
					players[up].status = PLAYER_STATUS::JUSTCOLLIDEDBASE;
				}
			}
		
			if( PlayerCollideWithLevel( &players[up], collisionmap ))
			{
				if( players[up].vx < 40 && players[up].vx > -40 &&
					players[up].vy < 60 && players[up].vy > 0)
				{
					players[up].status = PLAYER_STATUS::LANDED;
				}
				else
				{
					// send a chat pkg! :P
					if( players[up].Team == SHIPZ_TEAM::RED )
					{
						red_team.frags--;
					}
					else
					{
						blue_team.frags--;
					}
					std::cout << "player " << players[up].name << " collided with rock at " << players[up].x << "," << players[up].y << std::endl;
					players[up].status = PLAYER_STATUS::JUSTCOLLIDEDROCK;
				}
			}
			int bulletresult = PlayerCollideWithBullet( &players[up], up+1, players );
			if( bulletresult != -1 )
			{
				std::cout << "player " << players[up].name << " collided with bullet at " << players[up].x << "," << players[up].y << std::endl;
				// the function returns -1 when a player didn't collide, so he must've collided
				// note down the bullet for removal and change the player status
				// NOTE: in a later stage we should report which player shot him and
				// update etc.
				if( bullets[bulletresult].owner == SHIPZ_TEAM::RED )
				{
					if( bullets[bulletresult].type == WEAPON_MINE &&
						players[up].Team == SHIPZ_TEAM::RED )
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
					if( bullets[bulletresult].type == WEAPON_MINE &&
						players[up].Team == SHIPZ_TEAM::BLUE )
					{
						blue_team.frags--;
					}
					else
					{
						blue_team.frags++;
					}
				}

				bullets[bulletresult].collide = true;
				
				players[up].status = PLAYER_STATUS::JUSTSHOT;
			}
		}
	}

}

void Server::SendUpdates() {
	// send all the stuff to all the players
	// S: 040 PLAYER BASESTATES TEAMSTATES (PSTAT PFRAME PX PY PVX PVY BULX BULY BULVX BULVY) x8
	if(number_of_players == 0) {
		return;
	}
	std::cout << "BEGIN UPDATE" << std::endl;
	sendbuf.Clear();
	sendbuf.Write8(SHIPZ_MESSAGE::UPDATE, "UPDATE");
	sendbuf.Write8(0, "plyr_placeholder");

	Uint32 basestates = 0;
	for( int bidx = 0; bidx < MAXBASES; bidx++ ) {
		if( bases[bidx].owner == SHIPZ_TEAM::RED ) {
			basestates |= (1 << (bidx *2));
		}
		if( bases[bidx].owner == SHIPZ_TEAM::BLUE ) {
			basestates |= (1 << (bidx *2 +1));
		}
	}
	sendbuf.Write32( basestates, "basestates");
	
	sendbuf.Write16( Sint16( red_team.bases ), "red_team.bases");
	sendbuf.Write16( Sint16( blue_team.bases ), "blue_team.bases");
	
	for( int wp = 0; wp < MAXPLAYERS; wp++ )
	{
		sendbuf.Write16( Sint16( players[wp].status ), "status");
		sendbuf.Write16( Sint16( players[wp].shipframe ), "frame");
		sendbuf.Write16( Sint16( players[wp].typing ), "typing");
		sendbuf.Write16( Sint16( players[wp].x ), "x");
		sendbuf.Write16( Sint16( players[wp].y ), "y");
		sendbuf.Write16( Sint16( players[wp].vx ), "vx");
		sendbuf.Write16( Sint16( players[wp].vy ), "vy");
		sendbuf.Write8(Uint8(players[wp].bullet_shot), "bullet_shot");
	
		if( players[wp].bullet_shot )
		{
			sendbuf.Write16( Sint16( players[wp].bulletshotnr ), "bullet_shot_nr");
			sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].type ), "type");
			if( bullets[players[wp].bulletshotnr].type == WEAPON_BULLET ||
				bullets[players[wp].bulletshotnr].type == WEAPON_MINE )
			{
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].x ), "x");
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].y ), "y");
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].vx ), "vx");
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].vy ), "vy");
			}
			if( bullets[players[wp].bulletshotnr].type == WEAPON_ROCKET )
			{
				sendbuf.Write16( 0);
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].angle ), "angle");
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].x ), "x");
				sendbuf.Write16( Sint16( bullets[players[wp].bulletshotnr].y ), "y");
			}
			players[wp].bullet_shot = 0;
		}
	}
	
	Sint16 bulcount = 0;
	for( int cnt = 0; cnt < NUMBEROFBULLETS; cnt++ )
	{
		if( bullets[cnt].active == true && bullets[cnt].collide == true )
		{
			bulcount++;
		}
	}

	sendbuf.Write16( Sint16( bulcount ), "bulcount");

	for( Sint16 wrb = 0; wrb < NUMBEROFBULLETS; wrb++ )
	{
		if( bullets[wrb].active == true && bullets[wrb].collide == true )
		{
			sendbuf.Write16( (Sint16)wrb, "wrb");
			CleanBullet( int( wrb ));
		}
	}


	std::cout << "END UPDATE" << std::endl << std::endl;;
	
	for( int sp = 0; sp < MAXPLAYERS; sp++ )
	{
		if( players[sp].playing )
		{
			sendbuf.SetPosByte(1, sp + 1);
			SendBuffer(players[sp].playaddr);
		
			// deal with stati
			if(players[sp].status == PLAYER_STATUS::JUSTCOLLIDEDBASE)
			{
				players[sp].status = PLAYER_STATUS::DEAD;
			}
			if(players[sp].status == PLAYER_STATUS::JUSTCOLLIDEDROCK)
			{
				players[sp].status = PLAYER_STATUS::DEAD;
			}
			if( players[sp].status == PLAYER_STATUS::JUSTSHOT )
			{
				players[sp].status = PLAYER_STATUS::DEAD;
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
				case SHIPZ_MESSAGE::LEAVE:
					this->HandleLeave();
					break;
				case SHIPZ_MESSAGE::UPDATE:
					this->HandleUpdate();
					break;
				case SHIPZ_MESSAGE::JOIN:
					this->HandleJoin();
					break;
				case SHIPZ_MESSAGE::STATUS:
					this->HandleStatus();
					break;
				case SHIPZ_MESSAGE::CHAT:
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
		if(CheckVictory() != SHIPZ_TEAM::NEUTRAL) {
			auto event = new EventTeamWins(CheckVictory());
			SendEvent(event);
			done = true;
			runstate = SERVER_RUNSTATE_QUIT;

		} else if((float(SDL_GetTicks()) - lastsendtime) > SEND_DELAY)
		{
			this->SendUpdates();
		}
	}
}

void Server::SendEvent(Event *event) {
	this->sendbuf.Clear();
	this->sendbuf.Write8(SHIPZ_MESSAGE::EVENT);
	if(!event->Serialize(&this->sendbuf)) {
		throw new std::runtime_error("Insufficient buffer for sending event");
	}
	for(uint p = 0; p < MAXPLAYERS; p++ ) {
		SendBuffer(players[p].playaddr);
	}
}
