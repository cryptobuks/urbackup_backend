/*************************************************************************
*    UrBackup - Client/Server backup system
*    Copyright (C) 2011  Martin Raiber
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef CLIENT_ONLY

#include "../../Interface/Server.h"

#include "FileClient.h"

#include "data.h"
#include "../../stringtools.h"

#include <iostream>
#include <memory.h>

#define SERVER_TIMEOUT_BACKUPPED 120000
#define SERVER_TIMEOUT 120000
#define LOG

const std::string str_tmpdir="C:\\Windows\\Temp";
extern std::string server_identity;


void Log(std::string str)
{
	Server->Log(str);
}

int curr_fnum=0;

bool setSockP(SOCKET sock)
{
#ifdef _WIN32
		DWORD dwBytesReturned = 0;
		BOOL bNewBehavior = FALSE;
		int status;

		// disable  new behavior using
		// IOCTL: SIO_UDP_CONNRESET
		#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
		status = WSAIoctl(sock, SIO_UDP_CONNRESET,
						&bNewBehavior, sizeof(bNewBehavior),
					NULL, 0, &dwBytesReturned,
					NULL, NULL);
		if (SOCKET_ERROR == status)
		{
			return false;
		}
#endif
        return true;
}

_i32 selectnull(SOCKET socket)
{
        fd_set fdset;

        FD_ZERO(&fdset);

        FD_SET(socket, &fdset);

        timeval lon;
        lon.tv_usec=0;
        lon.tv_sec=0;
        return select((int)socket+1, &fdset, 0, 0, &lon);
}

_i32 selectmin(SOCKET socket)
{
        fd_set fdset;

        FD_ZERO(&fdset);

        FD_SET(socket, &fdset);

        timeval lon;
        lon.tv_sec=0;
        lon.tv_usec=10000;
        return select((int)socket+1, &fdset, 0, 0, &lon);
}

_i32 selectc(SOCKET socket, int usec)
{
        fd_set fdset;

        FD_ZERO(&fdset);

        FD_SET(socket, &fdset);

        timeval lon;
        lon.tv_sec=0;
        lon.tv_usec=usec;
        return select((int)socket+1, &fdset, 0, 0, &lon);
}
       

FileClient::FileClient(void)
{
        udpsock=socket(AF_INET,SOCK_DGRAM,0);

        setSockP(udpsock);

        BOOL val=TRUE;
        setsockopt(udpsock, SOL_SOCKET, SO_BROADCAST, (char*)&val, sizeof(BOOL) );      

        socket_open=false;
}

FileClient::~FileClient(void)
{
	if(socket_open && tcpsock!=NULL)
	{
		Server->destroy(tcpsock);
	}
	closesocket(udpsock);
}

std::vector<sockaddr_in> FileClient::getServers(void)
{
        return servers;
}

std::vector<std::wstring> FileClient::getServerNames(void)
{
        return servernames;
}

std::vector<sockaddr_in> FileClient::getWrongVersionServers(void)
{
        return wvservers;
}

_u32 FileClient::getLocalIP(void)
{
        return local_ip;
}

_u32 FileClient::GetServers(bool start, const std::vector<in_addr> &addr_hints)
{
        if(start==true)
        {
				max_version=0;
#ifdef _WIN32                
                //get local ip address
                char hostname[MAX_PATH];
                struct    hostent* h;
                _u32     address;

                _i32 rc=gethostname(hostname, MAX_PATH);
                if(rc==SOCKET_ERROR)
                        return 0;

                std::vector<_u32> addresses;

                if(NULL != (h = gethostbyname(hostname)))
                {
                for(_u32 x = 0; (h->h_addr_list[x]); x++)
                {
               
                        ((uchar*)(&address))[0] = h->h_addr_list[x][0];
                        ((uchar*)(&address))[1] = h->h_addr_list[x][1];
                        ((uchar*)(&address))[2] = h->h_addr_list[x][2];
                        ((uchar*)(&address))[3] = h->h_addr_list[x][3];
                        ((uchar*)(&address))[3]=255;
                        addresses.push_back(address);
                        local_ip=address;
                }
                }

                sockaddr_in addr_udp;

                ((uchar*)(&address))[0]=255;
                ((uchar*)(&address))[1]=255;
                ((uchar*)(&address))[2]=255;
                ((uchar*)(&address))[3]=255;

                addr_udp.sin_family=AF_INET;
                addr_udp.sin_port=htons(UDP_PORT);
                addr_udp.sin_addr.s_addr=address;

                char ch=ID_PING;
                sendto(udpsock, &ch, 1, 0, (sockaddr*)&addr_udp, sizeof(sockaddr_in) );

                for(size_t i=0;i<addresses.size();++i)
                {
                        addr_udp.sin_addr.s_addr=addresses[i];
                        sendto(udpsock, &ch, 1, 0, (sockaddr*)&addr_udp, sizeof(sockaddr_in) );
                }

				for(size_t i=0;i<addr_hints.size();++i)
				{
					addr_udp.sin_addr.s_addr=addr_hints[i].s_addr;
					sendto(udpsock, &ch, 1, 0, (sockaddr*)&addr_udp, sizeof(sockaddr_in) );
				}
#else
		int broadcast=1;
		if(setsockopt(udpsock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int))==-1)
		{
			Server->Log("Error setting socket to broadcast", LL_ERROR);
		}
		
		sockaddr_in addr_udp;
		addr_udp.sin_family=AF_INET;
		addr_udp.sin_port=htons(UDP_PORT);
		addr_udp.sin_addr.s_addr=inet_addr("255.255.255.255");
		memset(addr_udp.sin_zero,0, sizeof(addr_udp.sin_zero));
		
		char ch=ID_PING;
		int rc=sendto(udpsock, &ch, 1, 0, (sockaddr*)&addr_udp, sizeof(sockaddr_in));
		if(rc==-1)
		{
			Server->Log("Sending broadcast failed!", LL_ERROR);
		}

		broadcast=0;
		if(setsockopt(udpsock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int))==-1)
		{
			Server->Log("Error setting socket to not broadcast", LL_ERROR);
		}

		for(size_t i=0;i<addr_hints.size();++i)
		{
			addr_udp.sin_addr.s_addr=addr_hints[i].s_addr;
			sendto(udpsock, &ch, 1, 0, (sockaddr*)&addr_udp, sizeof(sockaddr_in) );
		}
#endif

                starttime=Server->getTimeMS();

                servers.clear();
				servernames.clear();
                wvservers.clear(); 


                return ERR_CONTINUE;
        }
        else
        {
            _i32 rc = selectmin( udpsock );

        	if(rc>0)
	        {
		        socklen_t addrsize=sizeof(sockaddr_in);
		        sockaddr_in sender;
		        _i32 err = recvfrom(udpsock, buffer, BUFFERSIZE_UDP, 0, (sockaddr*)&sender, &addrsize);
		        if(err==SOCKET_ERROR)
		        {
			        return ERR_ERROR;
        		}
                        if(err>2&&buffer[0]==ID_PONG)
                        {
                                int version=(unsigned char)buffer[1];
                                if(version==VERSION)
                                {
                                        servers.push_back(sender);

                                        std::string sn;
                                        sn.resize(err-2);
                                        memcpy((char*)sn.c_str(), &buffer[2], err-2);

										servernames.push_back(Server->ConvertToUnicode(sn));
                                }
                                else
                                {
                                        wvservers.push_back(sender);
                                }
                                
                                if( version>max_version )
                                {
                                        max_version=version;
                                }
                        }
                }

                if(Server->getTimeMS()-starttime>1000)
                {
                        return ERR_TIMEOUT;
                }
                else
                        return ERR_CONTINUE;

        }

        
}

int FileClient::getMaxVersion(void)
{
        return max_version;        
}

_u32 FileClient::Connect(sockaddr_in *addr)
{
	if( socket_open==true )
    {
            Server->destroy(tcpsock);
    }

	tcpsock=Server->ConnectStream(inet_ntoa(addr->sin_addr), TCP_PORT, 10000);

	if(tcpsock!=NULL)
	{
		socket_open=true;
	}

	server_addr=*addr;

    if(tcpsock==NULL)
		return ERR_ERROR;
	else
		return ERR_CONNECTED;
}       

_u32 FileClient::GetGameList(void)
{

		CWData data;
		data.addUChar(ID_GET_GAMELIST);
		data.addString( server_identity );

		stack.reset();
		stack.Send(tcpsock, data.getDataPtr(), data.getDataSize() );

		starttime=Server->getTimeMS();
		num_games_res=false;
		num_games_get=0;

		while(true)
		{
			size_t rc=tcpsock->Read(buffer, BUFFERSIZE_UDP, 10000);

			if(rc==0)
				return ERR_ERROR;

			starttime=Server->getTimeMS();

            stack.AddData(buffer, rc);
                                
			char *packet;
			size_t packetsize;
			while( (packet=stack.getPacket(&packetsize) )!=NULL )
			{
					if( packetsize>1 && packet[0]==ID_GAMELIST && num_games_res==false)
					{
							CRData data(&packet[1], packetsize);

							if( !data.getUInt(&num_games) )
							{
									delete [] packet;
									return ERR_ERROR;
							}

							res_name=true;
							num_games_res=true;
							num_games_get=0;

							if( num_games==0 )
							{
									delete [] packet;
									return ERR_SUCCESS;
							}
					}
					else if( num_games_res==true )
					{
							if( res_name==true )
							{
									games.push_back(&packet[0]);
									res_name=false;
							}
							else
							{
									writestring(packet, (unsigned int)packetsize, str_tmpdir+conv_filename(mServerName+"-"+games[ games.size()-1]) );
									res_name=true;
									num_games_get++;
									if( num_games_get==num_games )
									{
											delete [] packet;
											return ERR_SUCCESS;
									}
							}
					}
					delete []packet;
			}
               
			if( Server->getTimeMS()-starttime>10000)
            {
                    return ERR_TIMEOUT;
            }
        }
}

bool FileClient::ListDownloaded(void)
{
        if( num_games_get==num_games )
        {
                return true;
        }
        else
                return false;
}

std::vector<std::string> FileClient::getGameList(void)
{
        return games;
}

void FileClient::setServerName(std::string pName)
{
        mServerName=pName;
}

std::string FileClient::getServerName(void)
{
        return mServerName;
}

bool FileClient::isConnected(void)
{
        return connected;
}

bool FileClient::Reconnect(void)
{
	Server->destroy(tcpsock);
	connect_starttime=Server->getTimeMS();

	for(size_t i=0;i<8;++i)
	{
		tcpsock=Server->ConnectStream(inet_ntoa(server_addr.sin_addr), TCP_PORT, 10000);
		if(tcpsock!=NULL)
		{
			socket_open=true;
			return true;
		}
	}
	socket_open=false;
	return false;
}

 _u32 FileClient::GetFile(std::string remotefn, IFile *file)
{
	if(tcpsock==NULL)
		return ERR_ERROR;

	int tries=50;

    CWData data;
    data.addUChar( ID_GET_FILE );
    data.addString( remotefn );
	data.addString( server_identity );

    stack.Send( tcpsock, data.getDataPtr(), data.getDataSize() );

	_u64 filesize;
	_u64 received=0;
	bool firstpacket=true;

	if(file==NULL)
		return ERR_ERROR;

    starttime=Server->getTimeMS();

	char buf[BUFFERSIZE];

	while(true)
	{        
		size_t rc=tcpsock->Read(buf, BUFFERSIZE, 120000);

        if( rc==0 )
        {
			bool b=Reconnect();
			--tries;
			if(!b || tries<=0 )
			{
				return ERR_TIMEOUT;
			}
			else
			{
				CWData data;
				data.addUChar( ID_GET_FILE );
				data.addString( remotefn );
				data.addString( server_identity );
				if( firstpacket==false )
					data.addInt64( received ); 

				stack.Send( tcpsock, data.getDataPtr(), data.getDataSize() );
				starttime=Server->getTimeMS();
			}
		}
        else
        {
			starttime=Server->getTimeMS();

			_u32 off=0;
			uchar PID=buf[0];
                        
            if( firstpacket==true)
            {
                    if(PID==ID_COULDNT_OPEN)
                    {
                        return ERR_FILE_DOESNT_EXIST;
                    }
					else if(PID==ID_BASE_DIR_LOST)
					{
						return ERR_BASE_DIR_LOST;
					}
                    else if(PID==ID_FILESIZE && rc >= 1+sizeof(_u64))
                    {
                            memcpy(&filesize, buf+1, sizeof(_u64) );
                            off=1+sizeof(_u64);

                            if( filesize==0 )
                            {
                                    return ERR_SUCCESS;
                            }
                    }
                    firstpacket=false;
            }

            if( (_u32) rc > off )
            {
					_u32 written=off;
					while(written<rc)
					{
						written+=file->Write(&buf[written], (_u32)rc-written);
						if(written<rc)
						{
							Server->Log("Failed to write to file... waiting...", LL_WARNING);
							Server->wait(10000);
						}
					}

                    received+=rc-off;

					if( received >= filesize)
                    {
						return ERR_SUCCESS;
					}
            }
		}
            
	    if( Server->getTimeMS()-starttime > SERVER_TIMEOUT )
		{
				bool b=Reconnect();
				--tries;
				if(!b || tries<=0 )
				{
					return ERR_TIMEOUT;
				}
				else
				{
					CWData data;
					data.addUChar( ID_GET_FILE );
					data.addString( remotefn );
					data.addString( server_identity );
					if( firstpacket==false )
						data.addInt64( received ); 

					stack.Send( tcpsock, data.getDataPtr(), data.getDataSize() );
					starttime=Server->getTimeMS();
				}
		}
	}
}
        
        
        
#endif //CLIENT_ONLY

