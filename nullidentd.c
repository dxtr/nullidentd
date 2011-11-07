#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define SESSION_TIMEOUT 20

#define MAX_RESPONSE 200
#define MAX_REQUEST 100

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
	int data_read=0;
	char *p = buffer;
	if (!buffer || size < 1) return -1;
	FILE *randfd = fopen("/dev/urandom", "r");
	if (!randfd) { /* Fall back to /dev/random */
		randfd = fopen("/dev/random", "r");
		if (!randfd) return -1;
	}

	for (size_t i = 0; i < size/2; i++) {
		int c = fgetc(randfd);
		if (ferror(randfd) != 0 || feof(randfd) != 0) { break; }

		sprintf(p, "%x", c);
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
	char		response[MAX_RESPONSE];
	char		request[MAX_REQUEST];
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
		/* read the request */
		if( !read_request( infd, request, MAX_REQUEST ) ) {
			/* error or timed out */
			goto done;
		}

		/* format the response */
		memset(data, 0, sizeof(data));
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

