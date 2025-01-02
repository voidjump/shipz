#include <string>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "client.h"
#include "types.h"
#include "player.h"
#include "other.h"
#include "net.h"
#include "gfx.h"
#include "sound.h"
#include "font.h"
#include "event.h"
#include "team.h"
#include "base.h"
#include "assets.h"
#include "level.h"

// TODO:
// Candidates for bitwise state flags

// Construct a client
Client::Client(const char *server_address, const char * player_name) {
	this->server_address = server_address;
	this->name = player_name;
}

void Client::ResolveHostname(const char * connect_address) {
	this->ipaddr = SDLNet_ResolveHostname(connect_address);
	if( this->ipaddr == NULL) {
		FailErr("could not resolve server address");
	}
	
	// wait for the hostname to resolve:
	std::cout << "resolving hostname" << std::endl;
	while(SDLNet_GetAddressStatus(this->ipaddr) == 0) {
		std::cout << ".";
		SDL_Delay(1000);
	}

	if( SDLNet_GetAddressStatus(this->ipaddr) != 1) {
		FailErr("could not resolve server address");
	}

	// Resolved address
	std::cout << "resolved server address:" << SDLNet_GetAddressString(this->ipaddr) << std::endl;
}

void Client::CreateUDPSocket() {
	this->udpsock = SDLNet_CreateDatagramSocket(NULL, PORT_CLIENT);

	if(this->udpsock == NULL) {
		SDLNet_DestroyDatagramSocket(udpsock);
		FailErr("Failed to open socket");
	}
}

// Send Buffer to server
void Client::SendBuffer() {
	std::cout << "Sending buffer:";
	PrintRawBytes(this->send_buffer.AsString(), this->send_buffer.length);
	std::cout << std::endl;
	if (! SDLNet_SendDatagram(this->udpsock, 
							  this->ipaddr, 
							  PORT_SERVER, 
							  (void *)this->send_buffer.data, 
							  this->send_buffer.length)) {
		// proper failure
		SDLNet_DestroyDatagramSocket(this->udpsock);
		FailErr("Cannot send buffer to server");
	}
}

// Connect to the Server
void Client::Connect(const char *connect_address) {
	this->ResolveHostname(connect_address);
	this->CreateUDPSocket();

	// Send a status message
	this->send_buffer.Clear();
	this->send_buffer.Write8(SHIPZ_MESSAGE::STATUS);
	this->send_buffer.Write8(SHIPZ_VERSION);
	this->send_buffer.Write8('\0');
	
	std::cout << "@ querying server status.." << std::endl;
	
	while( this->notreadytocontinue && this->attempts < 5 && this->error == 0)
	{
		// send the following package according to protocol:
		// 020 VERSION
		this->SendBuffer();

		if (WaitForPacket(this->udpsock, &this->in)) {
			// got a response.

			// TODO: Safely read from buffer
			if( in->buflen > 0 && in->buf[0] == SHIPZ_MESSAGE::STATUS )
			{
				if( in->buf[1] == SHIPZ_VERSION )
				{
					std::cout << "@ server responded..." << std::endl;
					if( in->buf[2] < in->buf[3] )
					{
						this->number_of_players = in->buf[2];
						lvl.m_levelversion = in->buf[4];
						// TODO: This should be a safe copy..
						lvl.SetFile((const char*)&in->buf[5]);
						// proceed with joining
						this->notreadytocontinue = 0;	
						this->attempts = 0; // make sure we don't confuse stuff
					}
					else
					{
						// error, server full. try another.
						this->error = 3;
					}
				}
				else
				{
					// error, version mismatch. please up/downgrade.
					this->error = 2;
				}
			}
			else
			{
				// error, protocol error, please try to reconnect.
				this->error = 1;
			}
			// Free packet
			SDLNet_DestroyDatagram(this->in);
		}
		this->attempts++;
	}
	if( this->attempts == 5 )
	{
		this->error = 4;
	}
	if( this->error != 0 )
	{	
		SDLNet_DestroyDatagramSocket(this->udpsock);
		SDLNet_Quit();
		// return this->error;
	}
}

// Initialize some state
// Empty players
// Initialize SDL_Net
void Client::Init() {
	for( int ip = 0; ip < MAXPLAYERS; ip++ )
	{
		EmptyPlayer( &players[ ip ] );
	}
	
	//initiate SDL_NET
	if(!SDLNet_Init())
	{
		FailErr("failed to initialize SDLNet");
	}
}

void Client::HandleInputs() {
	SDL_Event event;
	while ( SDL_PollEvent(&event) )
	{
		if ( event.type == SDL_EVENT_QUIT ) {
			done = true;
		}
		if ( event.type == SDL_EVENT_KEY_DOWN )
		{
			if ( event.key.key == SDLK_ESCAPE ) {
				done = true;
			}
			if ( event.key.key == SDLK_TAB && !self->typing ) {
				switch( self->weapon )
				{
					case BULLET:
						self->weapon = ROCKET;
						break;
					case ROCKET:
						self->weapon = MINE;
						break;
					case MINE:
						self->weapon = BULLET;
						break;
				}
				PlaySound( weaponswitch );
			}
			if( event.key.key == SDLK_S && !self->typing )
			{
				this->TakeScreenShot();
			}

			GetTyping( &type_buffer, event.key.key, event.key.mod );
			
			if ( event.key.key == SDLK_RETURN )
			{
				if( self->typing ) {
					this->SendChatLine();
				}
				else {
					this->type_buffer.Clear();
					this->type_buffer.WriteString(self->name);
					this->type_buffer.WriteString(":");
					self->typing = true;
				}
			}
		}
	}
	
	keys = SDL_GetKeyboardState(NULL);
	if( self->status == PLAYER_STATUS::FLYING )
	{
		if( keys[SDL_SCANCODE_RIGHT] )
		{
			//rotate player clockwise
			PlayerRot( self, 1 );
			input_given = 1;
		}
		if( keys[SDL_SCANCODE_LEFT] )
		{
			//rotate player counterclockwise
			PlayerRot( self, 0 );
			input_given = 1;
		}
		if( keys[SDL_SCANCODE_UP] )
		{
			PlayerThrust( self );
			input_given = 1;
		}
		if( keys[SDL_SCANCODE_RCTRL] || keys[SDL_SCANCODE_LCTRL] )
		{
			if( (SDL_GetTicks() - self->lastliftofftime) > LIFTOFFSHOOTDELAY )
			{
				if( self->weapon == BULLET )
				{
					if( (SDL_GetTicks() - self->lastshottime) > BULLETDELAY )
					{
						self->bulletshotnr = ShootBullet( self, my_player_nr );
						if( self->bulletshotnr != 6666 )
						{
							self->lastshottime = SDL_GetTicks();
							self->bullet_shot = 1;
						}
						else
						{
							self->bulletshotnr = 0;
						}
					}
				}
				if( self->weapon == ROCKET )
				{
					if( (SDL_GetTicks() - self->lastshottime) > ROCKETDELAY )
					{
						self->bulletshotnr = ShootBullet( self, my_player_nr );
						if( self->bulletshotnr != 6666 )
						{
							self->lastshottime = SDL_GetTicks();
							self->bullet_shot = 1;
						}
						else
						{
							self->bulletshotnr = 0;
						}
					}
				}
				if( self->weapon == MINE )
				{
					if( (SDL_GetTicks() - self->lastshottime) > MINEDELAY )
					{
						self->bulletshotnr = ShootBullet( self, my_player_nr );
						if( self->bulletshotnr != 6666 )
						{
							self->lastshottime = SDL_GetTicks();
							self->bullet_shot = 1;
						}
						else
						{
							self->bulletshotnr = 0;
						}
					}

				}
			}
		}
	}
	
	if( keys[SDL_SCANCODE_UP] )
	{
		if( self->status == PLAYER_STATUS::LANDED || self->status == PLAYER_STATUS::LANDEDBASE )
		{
			self->status = PLAYER_STATUS::LIFTOFF;
			self->y -= 10;
			input_given = 1;
			self->lastliftofftime = SDL_GetTicks();
		}
		if( self->status == PLAYER_STATUS::LANDEDRESPAWN )
		{
			// no bullet delay after respawn, so don't reset lastliftofftime
			self->status = PLAYER_STATUS::LIFTOFF;
			self->y -= 10;
			input_given = 1;
		}
	}
	if( self->status == PLAYER_STATUS::DEAD )
	{
		if( keys[SDL_SCANCODE_SPACE] && !self->typing )
		{
			// respawn
			input_given = 1;
			//ResetPlayer( self );
			self->status = PLAYER_STATUS::RESPAWN;
			//UpdatePlayer(self);
		}
	}

	if( !self->typing )
	{
		if( keys[SDL_SCANCODE_X] )
		{
			if( self->status != PLAYER_STATUS::DEAD && self->status != PLAYER_STATUS::RESPAWN )
			{
				self->status = PLAYER_STATUS::SUICIDE;
				input_given = 1;
			}
		}
	}
}

bool Client::ReceivedPacket() {
	bool result = false;
	if(!SDLNet_ReceiveDatagram(udpsock, &in) || in == NULL) {
		// did not receive packet or it is invalid
		return false;
	}

	if(in->buflen == 0 || SDLNet_CompareAddresses(in->addr, ipaddr) != 0) {
		// packet is 0 byte or it is not addressed to us
		std::cout << "received packet not addressed to us" << std::endl;
		result = false;
		goto return_result;
	}

	// import packet to buffer
	this->receive_buffer.Clear();
	if(!this->receive_buffer.ImportBytes(in->buf, in->buflen)) {
		std::cout << "failed to import packet to buffer" << std::endl;
		result = false;
		goto return_result;
	} else {
		result = true;
	}

	return_result:
	SDLNet_DestroyDatagram(in);
	return result;
}

void Client::HandleKicked() {
	std::cout << "Kicked by server" << std::endl;
	done = true;
}

void Client::HandleUpdate() {
	// Legacy check
	if( my_player_nr != receive_buffer.Read8()) {
		return;
	}

	Uint32 basestates = receive_buffer.Read32();
	for( int bidx = 0; bidx < MAXBASES; bidx++ ) {
		if( basestates & (1 << (bidx *2))) {
			bases[bidx].owner = SHIPZ_TEAM::RED;
		} 
		if( basestates & (1 << (bidx *2 +1))) {
			bases[bidx].owner = SHIPZ_TEAM::BLUE;
		}
	}
	red_team.bases = (Sint16)receive_buffer.Read16();
	blue_team.bases = (Sint16)receive_buffer.Read16();

	for( int rp=0; rp < MAXPLAYERS; rp++ )
	{
		if( rp != (my_player_nr -1 ))
		{
			int tempstat = (Sint16) receive_buffer.Read16();

			players[rp].shipframe = (Sint16) receive_buffer.Read16();
			players[rp].typing = (Sint16) receive_buffer.Read16();
			players[rp].x = (Sint16) receive_buffer.Read16();
			players[rp].y = (Sint16) receive_buffer.Read16();
			players[rp].vx = (Sint16) receive_buffer.Read16();
			players[rp].vy = (Sint16) receive_buffer.Read16();
		
			if( tempstat == PLAYER_STATUS::FLYING && (players[rp].status == PLAYER_STATUS::LANDED ||
				players[rp].status == PLAYER_STATUS::LANDEDBASE))
			{
				players[rp].lastliftofftime = SDL_GetTicks();
			}
			if( tempstat == PLAYER_STATUS::DEAD && players[rp].status != PLAYER_STATUS::DEAD )
			{
				NewExplosion( int(players[rp].x),int( players[rp].y ));
			}
			players[rp].status = tempstat;

		
			Sint16 tx, ty, tvx, tvy, tn, tbultyp;
			tn = (Sint16) receive_buffer.Read16();
			tbultyp = (Sint16) receive_buffer.Read16();
			tx = (Sint16) receive_buffer.Read16();
			ty = (Sint16) receive_buffer.Read16();
			tvx = (Sint16) receive_buffer.Read16();
			tvy = (Sint16) receive_buffer.Read16();
			if( tx == 0 && ty == 0 && tvx == 0 && tvy == 0 && tn == 0 &&
				tbultyp == 0 )
			{
				// this is an empty Bullet
			}
			else
			{
				if( tbultyp == MINE )
				{
					bullets[tn].x = (float)tx;
					bullets[tn].y = (float)ty;
					bullets[tn].minelaidtime = SDL_GetTicks();
				}
				if( tbultyp == BULLET )
				{
					bullets[tn].x = (float)tx;
					bullets[tn].y = (float)ty;
					bullets[tn].vx = (float)tvx;
					bullets[tn].vy = (float)tvy;
				}
				if( tbultyp == ROCKET )
				{
					bullets[tn].x = (float)tvx;
					bullets[tn].y = (float)tvy;
					bullets[tn].angle = (float)ty;
					PlaySound( rocketsound );
				}
				bullets[tn].type = tbultyp;
				bullets[tn].active = 1;
				bullets[tn].collide = 0;
				bullets[tn].owner = rp+1;
			}
		}
		else
		{
			int tempstatus = (Sint16) receive_buffer.Read16();
			Sint16 tx, ty;
			tx = (Sint16)receive_buffer.Read16();
			ty = (Sint16)receive_buffer.Read16();
		
			if( tempstatus == PLAYER_STATUS::DEAD && self->status == PLAYER_STATUS::SUICIDE )
			{
				std::cout << "we have just suicided!" << std::endl;
				NewExplosion( int(self->x), int(self->y));
				self->status = PLAYER_STATUS::DEAD;
			}
			if( tempstatus == PLAYER_STATUS::JUSTCOLLIDEDROCK && self->status == PLAYER_STATUS::FLYING )
			{
				std::cout << "we just collided with a rock!" << std::endl;
				NewExplosion( int(self->x), int(self->y));
				self->status = PLAYER_STATUS::DEAD;
			}
			if( tempstatus == PLAYER_STATUS::JUSTCOLLIDEDBASE && self->status == PLAYER_STATUS::FLYING )
			{
				std::cout << "we just collided with Base!" << std::endl;
				NewExplosion( int(self->x), int(self->y));
				self->status = PLAYER_STATUS::DEAD;
			}
			if( tempstatus == PLAYER_STATUS::JUSTSHOT && self->status == PLAYER_STATUS::FLYING )
			{
				std::cout << "we were just shot!" << std::endl;
				NewExplosion( int(self->x), int(self->y));
				self->status = PLAYER_STATUS::DEAD;
			}
			if( tempstatus == PLAYER_STATUS::LANDEDRESPAWN && self->status == PLAYER_STATUS::RESPAWN )
			{
				std::cout << "server said we could respawn!" << std::endl;
				int tmpbs = FindRespawnBase( self->Team );

				// Base found, reset the player's speed etc.
				ResetPlayer( self );
				UpdatePlayer( self );
				// mount /dev/player /mnt/Base 
				self->x = bases[ tmpbs ].x;
				self->y = bases[ tmpbs ].y - 26;

				self->status = PLAYER_STATUS::LANDEDRESPAWN;
			}
			if( tempstatus == PLAYER_STATUS::FLYING && self->status == PLAYER_STATUS::LIFTOFF )
			{
				std::cout << "we are flyig!" << std::endl;
				self->status = PLAYER_STATUS::FLYING;
			}
			if( tempstatus == PLAYER_STATUS::LANDED && self->status == PLAYER_STATUS::FLYING ) 
			{
				std::cout << "we have landed!" << std::endl;
				self->status = PLAYER_STATUS::LANDED;
				self->vx = 0;
				self->vy = 0;
				self->engine_on = 0;
				self->flamestate = 0;
			}
			if( tempstatus == PLAYER_STATUS::LANDEDBASE && self->status == PLAYER_STATUS::FLYING )
			{
				std::cout << "we have landed on a Base!" << std::endl;
				int tmpbase = GetNearestBase( int(self->x), int(self->y));
										
				self->y = bases[tmpbase].y - 26;
				self->angle = 0;
				self->shipframe = 0;
				self->status = PLAYER_STATUS::LANDEDBASE;
				self->vx = 0;
				self->vy = 0;
				self->engine_on = 0;
				self->flamestate = 0;
				UpdatePlayer(self);
			}
		}
	}
	
	Sint16 tmpval = 0;
	tmpval = (Sint16)receive_buffer.Read16();
	if( tmpval != 0 )
	{
		for( int gcb = 0; gcb < tmpval; gcb++ )
		{
			Sint16 num = (Sint16)receive_buffer.Read16();
			if( bullets[num].type == MINE || bullets[num].type == ROCKET )
			{
				NewExplosion( int(bullets[num].x), int(bullets[num].y));
			}
			CleanBullet( int( num ) );
		}
	}
}

void Client::HandleChat() {
	// Legacy check
	if( my_player_nr != receive_buffer.Read8()) {
		return;
	}
	// chat package
	memset( chat1, '\0', sizeof( chat1 ));
	strcpy( chat1, chat2 );

	memset( chat2, '\0', sizeof( chat2 ));
	strcpy( chat2, chat3 );

	memset( chat3, '\0', sizeof( chat3 ));
	receive_buffer.ReadStringCopyInto(chat3, sizeof(chat3));
}

void Client::HandlePlayerJoins() {
	// Legacy check
	if( my_player_nr != receive_buffer.Read8()) {
		return;
	}
	Uint8 new_player_id = receive_buffer.Read8() - 1;
	Uint8 new_player_team = receive_buffer.Read8();
	// a player joined
	number_of_players++;
	players[new_player_id].playing = 1;
	players[new_player_id].self_sustaining = 1;
	players[new_player_id].Team = new_player_team;
	receive_buffer.ReadStringCopyInto(players[new_player_id].name, 12);
	InitPlayer( &players[new_player_id] );
}

void Client::HandlePlayerLeaves() {
	// Legacy check
	if( my_player_nr != receive_buffer.Read8()) {
		return;
	}
	Uint8 player_leave_id = receive_buffer.Read8() - 1;
	// a player leaves
	number_of_players--;
	players[player_leave_id].playing = 0;
	players[player_leave_id].self_sustaining = 0;
	InitPlayer( &players[player_leave_id] );
}

void Client::HandleEvent() {
	std::cout << "handling event" << std::endl;
	// Somehow this is not being handled correctly
	// Maybe write a unit test?
	Event * event = Event::Deserialize(&receive_buffer);
	if( event == NULL )	 {
		std::cout << "event = null?" << std::endl;
		return;
	}
	switch( event->event_type ) {
		case TEAM_WINS:
			std::cout << "Team " << static_cast<EventTeamWins*>(event)->team << " wins!" << std::endl;
			done = true;
			break;
		default:
			std::cout << "Unimplemented event type" << std::endl;
	}
}

void Client::UpdatePlayers() {
	for( int up = 0; up < 8; up++ )
	{
		if( players[up].playing )
		{
			if( players[up].status == PLAYER_STATUS::FLYING )
			{
				UpdatePlayer( &players[up] );
			}
		}
	}
}

void Client::Tick() {
	oldtime = newtime;
	newtime = float(SDL_GetTicks());
	deltatime = newtime - oldtime;
}

void Client::GameLoop() {
	while(!done)
	{
		HandleInputs();
	
		Tick();

		if(ReceivedPacket()) {
			Uint8 protocol_header = receive_buffer.Read8();
			switch(protocol_header) {
				case SHIPZ_MESSAGE::KICK:
					std::cout << "received kick notice" << std::endl;
					HandleKicked();
					break;
				case SHIPZ_MESSAGE::UPDATE:
					std::cout << "received update" << std::endl;
					HandleUpdate();
					break;
				case SHIPZ_MESSAGE::CHAT:
					std::cout << "received chat" << std::endl;
					HandleChat();
					break;
				case SHIPZ_MESSAGE::MSG_PLAYER_JOINS:
					std::cout << "received joins" << std::endl;
					HandlePlayerJoins();
					break;
				case SHIPZ_MESSAGE::MSG_PLAYER_LEAVES:
					std::cout << "received leaves" << std::endl;
					HandlePlayerLeaves();
					break;
				case SHIPZ_MESSAGE::EVENT:
					std::cout << "received event" << std::endl;
					HandleEvent();
					break;
				default:
					std::cout << "received unknown packet type" << std::endl;
					break;
			}
		}

		UpdatePlayers();
		UpdateBullets( players );
		ClearOldExplosions();
				
		if( (SDL_GetTicks() - lastsendtime) > SEND_DELAY )
		{
			SendUpdate();
		}
		Draw();
	}
	Leave();
}

// should send the following stuff:
// 040 PLAYER STATUS ANGLE X Y VX VY WEAPON NUMBULS ( BULX BULY BULVX BULVY * NUMBULS )
void Client::SendUpdate() {
	this->send_buffer.Clear();

	this->send_buffer.Write8(SHIPZ_MESSAGE::UPDATE);
	this->send_buffer.Write8(my_player_nr);
	this->send_buffer.Write8(self->status);

	this->send_buffer.Write16( Sint16( self->shipframe ));
	this->send_buffer.Write16( Sint16( self->typing ));
	this->send_buffer.Write16( Sint16( self->x ));
	this->send_buffer.Write16( Sint16( self->y ));
	this->send_buffer.Write16( Sint16( self->vx ));
	this->send_buffer.Write16( Sint16( self->vy ));

	if( self->bullet_shot )
	{
		this->send_buffer.Write16( Sint16(self->bulletshotnr));
		this->send_buffer.Write16( Sint16( bullets[self->bulletshotnr].type));
		if( bullets[self->bulletshotnr].type == BULLET ||
			bullets[self->bulletshotnr].type == MINE )
		{
			this->send_buffer.Write16( Sint16( bullets[self->bulletshotnr].x));
			this->send_buffer.Write16( Sint16( bullets[self->bulletshotnr].y));
			this->send_buffer.Write16( Sint16( bullets[self->bulletshotnr].vx));
			this->send_buffer.Write16( Sint16( bullets[self->bulletshotnr].vy));
		}
		if( bullets[self->bulletshotnr].type == ROCKET)
		{
			// Why is this here?
			this->send_buffer.Write16( 0 );
			this->send_buffer.Write16( Sint16( bullets[self->bulletshotnr].angle));
			this->send_buffer.Write16( Sint16( bullets[self->bulletshotnr].x));
			this->send_buffer.Write16( Sint16( bullets[self->bulletshotnr].y));

		}
		self->bullet_shot = false;
		self->bulletshotnr = 0;
	}
	else
	{
		this->send_buffer.WriteBytes(12, 0x00);
	}

	this->SendBuffer();

	input_given = false;
	lastsendtime = SDL_GetTicks();
}

// Run the game
void Client::Run() {
	this->Connect(this->server_address);
	this->Join();
	this->Load();
	this->GameLoop();
}

// Fail with a human readable error
void Client::FailErr(const char * msg) {
	std::cout << msg << std::endl;
	std::cout << SDL_GetError() << std::endl;
	
	// TODO: Cleanup state
	SDLNet_Quit();
	exit(1);
}

// Wait for a packet for a while
bool Client::WaitForPacket(SDLNet_DatagramSocket * sock, SDLNet_Datagram ** dgram) {
	SDL_Delay(1000);
	return SDLNet_ReceiveDatagram(sock, dgram) && *dgram != NULL;
}

// Join a server
bool Client::Join() {

	std::cout << "@ joining..." << std::endl;
		
	// server is there, everything is OK proceed with joining
	// send the following package according to protocol:
	// 030 DNAME
	this->send_buffer.Clear();
	this->send_buffer.Write8(SHIPZ_MESSAGE::JOIN);
	this->send_buffer.WriteString(this->name);
	this->send_buffer.Write8('\0');

	this->SendBuffer();
	
	in = NULL;
	if (WaitForPacket(udpsock, &in))
	{
		if( in->buf[0] == SHIPZ_MESSAGE::JOIN )
		{
			my_player_nr = in->buf[1];
			std::cout << "Joined as player " << this->name << std::endl;
			self = &players[my_player_nr-1];
			Uint8 * tmpptr = &in->buf[2];	
			for( int player_index = 0; player_index < MAXPLAYERS; player_index++ )
			{
				int tmpteam = (int) Read16( tmpptr );
				if( tmpteam != 0 )
				{
					players[player_index].playing = 1;
					players[player_index].Team = tmpteam;
				}
				else
				{
					players[player_index].playing = 0;
				}

				if( players[player_index].playing == 1 )
				{
					players[player_index].self_sustaining = 1;
					InitPlayer( &players[player_index] );
				}
				tmpptr+=2;
				strncpy( players[player_index].name, (const char*)tmpptr, 12 );
				tmpptr+=12;
			}
		}
		else
		{
			// error, protocol error, please try to reconnect.
			error = 1;
		}
		// Free packet
		SDLNet_DestroyDatagram(in);
	}
	else
	{	
		// error: server timeout
		error = 4;
	}
	
	if( error != 0 )
	{
		SDLNet_DestroyDatagramSocket(udpsock);
		SDLNet_Quit();
		return error;
	}
	return 0;
}

void Client::Leave() {
	// send we are leaving
	this->send_buffer.Clear();
	this->send_buffer.Write8(SHIPZ_MESSAGE::LEAVE);
	this->send_buffer.Write8(my_player_nr);
	this->send_buffer.Write8('\0');
	this->SendBuffer();
}

void Client::Load() {
	if( !lvl.Load() ) 
	{	
		std::cout << "failed to load level!" << std::endl;
		error = 5;
		SDLNet_DestroyDatagramSocket(udpsock);
		SDLNet_Quit();
		// return error;
		exit(1);
	}

	InitSound();
	InitFont();
	LoadAssets();	
	CreateGonLookup();
	
	self->self_sustaining = 0;
	self->playing = 1;
	self->x = 320;
	self->y = 300;
	self->status = PLAYER_STATUS::DEAD; 
	
	memset ( chat1, '\0', sizeof( chat1 ));
	memset ( chat2, '\0', sizeof( chat2 ));
	memset ( chat3, '\0', sizeof( chat3 ));
	
	for( int zb = 0; zb < NUMBEROFBULLETS; zb++ )
	{
		CleanBullet( zb );
	}
	
	CleanAllExplosions();
}

// Perform UI/Game drawing
void Client::Draw() {
	AdjustViewport( this->self ); // focus viewport on self
				// maybe in later stage we could use this for spectator mode..
				// so that when player is dead he can view another (friendly?) player.

	Slock( screen );
	DrawIMG(level, 0, 0, XRES, YRES, viewportx, viewporty);
	
	DrawBullets( bulletpixmap );
		
	for( int up = 0; up < 8; up++ )
	{
		// if ship = 1... etc.. do later
		if( players[up].playing && players[up].status != PLAYER_STATUS::DEAD && players[up].status != PLAYER_STATUS::RESPAWN )
		{
			if( players[up].Team == SHIPZ_TEAM::RED )
			{
				DrawPlayer( shipred, &players[up] );
			}
			if( players[up].Team == SHIPZ_TEAM::BLUE )
			{
				DrawPlayer( shipblue, &players[up] );
			}
		}
	}
	DrawBases( basesimg );
	if( self->status == PLAYER_STATUS::DEAD )
	{
		DrawFont( sansboldbig, "Press space to respawn.", XRES-280, YRES-13, FONT_COLOR::WHITE  );
	}

	DrawExplosions();

	if( strlen(chat1) )
	{
		DrawFont( sansbold, chat1, 5, 5, FONT_COLOR::WHITE );
	}
	if( strlen(chat2) )
	{
		DrawFont( sansbold, chat2, 5, 16, FONT_COLOR::WHITE );
	}
	if( strlen(chat3) )
	{
		DrawFont( sansbold, chat3, 5, 27, FONT_COLOR::WHITE );
	}

	DrawIMG( scores, 0, YRES - 19 );
	char tempstr[10];
	// draw blue frags:
	snprintf( tempstr, 10, "%i", blue_team.frags );
	DrawFont( sansbold, tempstr, 4, YRES-17, FONT_COLOR::WHITE );
	// draw blue bases:
	snprintf( tempstr, 10, "%i", blue_team.bases );
	DrawFont( sansbold, tempstr, 29, YRES-17, FONT_COLOR::WHITE );
	
	// draw red frags:
	snprintf( tempstr, 10, "%i", red_team.frags );
	DrawFont( sansbold, tempstr, 54, YRES-17, FONT_COLOR::WHITE );
	
	// draw red bases:
	snprintf( tempstr,10, "%i", red_team.bases );
	DrawFont( sansbold, tempstr, 79, YRES-17, FONT_COLOR::WHITE );

	switch( self->weapon )
	{
		case BULLET:
			DrawIMG( bullet_icon, 110, YRES-18 );
			break;
		case ROCKET:
			DrawIMG( rocket_icon, 110, YRES-18 );
			break;
		case MINE:
			DrawIMG( mine_icon, 110, YRES-18 );
			break;
	}
	
	if( self->typing )
	{
		DrawFont( sansbold, type_buffer.AsString(), 5, YRES-26, FONT_COLOR::WHITE );
	}

	UpdateScreen();
	Sulock( screen );
}


Client::~Client() {
	SDL_DestroySurface( shipred );
	SDL_DestroySurface( shipblue );
	SDL_DestroySurface( chatpixmap );
	SDL_DestroySurface( level );
	SDL_DestroySurface( crosshairred );
	SDL_DestroySurface( crosshairblue );
	SDL_DestroySurface( bulletpixmap );
	SDL_DestroySurface( rocketpixmap );
	SDL_DestroySurface( minepixmap );
	SDL_DestroySurface( basesimg );
	SDL_DestroySurface( rocket_icon );
	SDL_DestroySurface( bullet_icon );
	SDL_DestroySurface( mine_icon );
	SDL_DestroySurface( scores );


	Mix_FreeChunk( explodesound );
	Mix_FreeChunk( rocketsound );
	Mix_FreeChunk( weaponswitch );
	Mix_CloseAudio();

	TTF_CloseFont( sansbold );
	TTF_CloseFont( sansboldbig );

	TTF_Quit();

	SDLNet_DestroyDatagramSocket(udpsock);
	SDLNet_Quit();
}

void Client::TakeScreenShot() {
	char tempstr[30];
	memset( tempstr, '\0', sizeof(tempstr));
	snprintf( tempstr, 30, "ss%d.bmp", screenshotcounter);
	SDL_SaveBMP(screen, tempstr);
	this->screenshotcounter++;
}


// This is called when a player presses return
// It transmits the current chat entered in the chat buffer to the server.
void Client::SendChatLine() {
	// Moves all chat rows up
	memset( chat1, '\0', sizeof( chat1 ));
	strcpy( chat1, chat2 );

	memset( chat2, '\0', sizeof( chat2 ));
	strcpy( chat2, chat3 );

	memset( chat3, '\0', sizeof( chat3 ));
	strcpy( chat3, this->type_buffer.AsString() );

	this->send_buffer.Clear();
	this->send_buffer.Write8(SHIPZ_MESSAGE::CHAT);
	this->send_buffer.Write8(this->my_player_nr);
	this->send_buffer.WriteString(this->type_buffer.AsString());

	this->SendBuffer();
	self->typing = false;
}