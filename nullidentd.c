#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define SESSION_TIMEOUT 20

int write_response( int fd, char *response, int len )
{
	int		retval;
	int		byteswritten = 0;

	while( byteswritten < len ) {
		retval = write( fd, response + byteswritten, len - byteswritten );
		if( retval <= 0 ) {
			/* error */
			return 0;
		}
		byteswritten += retval;
	}

	/* success */
	return 1;
}

int read_request( int fd, char *request, int maxlen )
{
	int		retval;
	char	c;
	int		bytesread = 0;

	/* read until \n */
	while( bytesread < maxlen - 1 ) {
		if( read( fd, &c, 1 ) != 1 ) {
			/* error */
			return 0;
		}
		if( c == '\n' ) {
			/* end of line */
			break;
		}

		request[bytesread] = c;
		bytesread += 1;
	}

	if( bytesread > 0 && request[bytesread-1] == '\r' ) {
		/* strip trailing \r */
		bytesread -= 1;
	}
	/* null terminate */
	request[bytesread] = '\0';

	/* success */
	return 1;
}

int read_random(char *buffer, size_t size)
{
	char *p = buffer;
	int data_read=0;
	if (!buffer || size < 2) return -1;
	FILE *randfd = fopen("/dev/urandom", "r");
	if (!randfd) { /* Fall back to /dev/random */
		randfd = fopen("/dev/random", "r");
		if (!randfd) return -1;
	}
	while (strlen(buffer) < size) {
		int c = fgetc(randfd);
		if (ferror(randfd) != 0 || feof(randfd) != 0) { break; }
		if (c < 1) continue;

		sprintf(p, "%02x", c);
		p += 2;
	}

ret:
	fclose(randfd);
	return data_read;

}

void session_timeout( int foo )
{
	exit( 0 );
}

int main( int argc, const char *argv[] )
{
	char		c;
	int			infd;
	int			outfd;
	int			response_len;
	char		response[256];
	char		request[129];
	char data[9];

	if( getgid() == 0 ) {
		fprintf( stderr, "Group id is root, exitting.\n" );
		return 1;
	}
	if( getuid() == 0 ) {
		fprintf( stderr, "User id is root, exiting.\n" );
		return 1;
	}

	infd = fileno( stdin );
	outfd = fileno( stdout );

	// set the session timeout
	signal( SIGALRM, session_timeout );
	alarm( SESSION_TIMEOUT );

	for( ;; ) {
		memset(response, 0, sizeof(response));
		memset(request, 0, sizeof(request));
		memset(data, 0, sizeof(data));
		/* read the request */
		if(!read_request(infd, request, sizeof(request))) {
			/* error or timed out */
			goto done;
		}

		/* format the response */
		read_random(data, 8);
		response_len = snprintf( response, sizeof( response ), "%.20s : USERID : UNIX : %.20s\r\n", request, data );
		/* send the line */
		if( !write_response( outfd, response, response_len ) ) {
			goto done;
		}
	}

done:
	return 0;
}

