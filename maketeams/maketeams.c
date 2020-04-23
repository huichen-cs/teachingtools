#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct _team {
	int *indices;	
	int size;
} team_t;

const int BUFFER_SIZE = 128;
const int CLASS_SIZE = 50;

static int readline(const int fd, char **buf);
static int readclass(const int infd, char ***names);
static int drawrandomteams(const int classsize, char *names[], 
	const int teamsize, team_t **teams);
static int printteams(const int outfd, const int numteams, team_t *teams, 
	char *names[]);
static void cleanup(const int size, char *names[], const int numteams, 
	team_t *teams);


int main(int argc, char *argv[]) {
	int infd, outfd, classsize, teamsize, numteams;
	char **names;
	team_t *teams;
	
	if (argc < 3) {
		fprintf(stderr, 
			"Usage: %s class_roster_file team_size out_team_file\n", 
			argv[0]);
		exit(EXIT_FAILURE);
	}

	infd = open(argv[1], O_RDONLY);
	if (infd == -1) {
		fprintf(stderr, 
			"Openning %s: %s\n", argv[1], strerror(errno));
		exit(EXIT_FAILURE);
	}

	sscanf(argv[2], "%d", &teamsize);	

	outfd = open(argv[3], O_WRONLY|O_CREAT, 0644);
	if (outfd == -1) {
		fprintf(stderr, 
			"Openning %s: %s\n", argv[2], strerror(errno));
		exit(EXIT_FAILURE);
	}

	if ((classsize = readclass(infd, &names)) == -1) {
		fprintf(stderr, "Failed to read class\n");
		exit(EXIT_FAILURE);
	}

	numteams = drawrandomteams(classsize, names, teamsize, &teams);
	
	if (printteams(outfd, numteams, teams, names) == -1) {
		fprintf(stderr, "Failed to write team data\n");
		exit(EXIT_FAILURE);
	}

	cleanup(classsize, names, numteams, teams);
	close(infd);
	close(outfd);
	return EXIT_SUCCESS;
}


static int readline(const int fd, char **buf) {
	int i;

	*buf = malloc(BUFFER_SIZE);
	if (*buf == NULL) return -1;
	i = 0;
	while (read(fd, *buf+i, 1) > 0 && *(*buf+i) != '\n') {
		if (*(*buf+i) == '\r') {
			continue;
		}	
		i ++;
		if (i >= BUFFER_SIZE) {
			free(*buf);
			return -1;
		} 
	} 
	*(*buf+i) = '\0';
	if (i == 0) free(*buf);
	return i;
}

static int readclass(const int infd, char ***names) {
	char *buf;
	int classsize = 0;

	*names = malloc(CLASS_SIZE*sizeof(char*));
	while (readline(infd, &buf) > 0) {
		(*names)[classsize ++] = buf;
		if (classsize > CLASS_SIZE) {
			return -1;
		}
	}
	return classsize;
}

static int drawrandomteams(const int classsize, char *names[], 
	const int teamsize, team_t **teams) {
	int numteams = classsize / teamsize, numdrawn, teamdrawn, selected, 
		*indices, *teamindices;

	srandom(time(NULL));

	/* select members randomly and add to teams */
	indices = malloc(classsize*sizeof(int));
	for (int i = 0; i < classsize; i ++) {
		indices[i] = i;
	}

	*teams = malloc(sizeof(team_t)*numteams);

	numdrawn = 0;
	for (int i = 0; i < numteams; i ++) {
		(*teams)[i].indices = malloc((teamsize+1)*sizeof(int));
		(*teams)[i].size = teamsize;
		for (int j = 0; j < teamsize; j++) {
			selected = random() % (classsize - numdrawn);
			(*teams)[i].indices[j] = indices[selected];
			indices[selected] = indices[classsize - numdrawn - 1];
			indices[classsize - numdrawn - 1] = -1;
			numdrawn ++;
		}
	}



	/* add remaining students to team randomly selected teams */
	teamindices = malloc(numteams*sizeof(int));
	for (int i = 0; i < numteams; i ++) {
		teamindices[i] = i;
	}

	teamdrawn = 0;
	for (int i = 0; i < classsize - numdrawn; i ++) {
		team_t *team;
		selected = random() % (numteams - teamdrawn);
		team = (*teams) + teamindices[selected];
		team->indices[team->size] = indices[i];
		team->size ++;
		teamindices[selected] = teamindices[numteams - teamdrawn - 1];
		teamindices[numteams - teamdrawn - 1] = -1;
		teamdrawn ++;
	}

	free(indices);
	free(teamindices);
	return numteams;
}

static int printteams(const int outfd, const int numteams, team_t *teams, 
	char *names[]) {
	char *buf = malloc(BUFFER_SIZE);
	for (int i = 0; i < numteams; i ++) {
		snprintf(buf, BUFFER_SIZE, "teams[%02d] = \n", i);
		if (write(outfd, buf, strlen(buf)) == -1) {
			free(buf);
			return -1;
		}
		for(int j = 0; j < teams[i].size; j ++ ) {
			snprintf(buf, BUFFER_SIZE, "\t%s\n", names[teams[i].indices[j]]);
			if (write(outfd, buf, strlen(buf)) == -1) {
				free(buf);
				return -1;
			}
		}
	}
	free(buf);
	return 0;
}

static void cleanup(const int size, char *names[], const int numteams, 
	team_t *teams) {
	for (int i = 0; i < size; i ++) {
		free(names[i]);
	}
	free(names);

	for (int i = 0; i < numteams; i ++) {
		free(teams[i].indices);	
	}
	free(teams);
}

