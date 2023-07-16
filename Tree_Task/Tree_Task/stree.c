
#define _XOPEN_SOURCE 600 /* Get nftw() */
#include <ftw.h>
#include <sys/types.h>    /* Type definitions used by many programs */
#include <stdio.h>        /* Standard I/O functions */
#include <stdlib.h>       /* Prototypes of commonly used library functions,
                             plus EXIT_SUCCESS and EXIT_FAILURE constants */
#include <unistd.h>       /* Prototypes for many system calls */
#include <errno.h>        /* Declares errno and defines error constants */
#include <string.h>       /* Commonly used string-handling functions */

#include <pwd.h>
#include <grp.h>

int decToOctal(int n)
{
  
    // array to store octal number
    int octalNum[3];
  
    // counter for octal number array
    int i = 0;
    while (n != 0) {
  
        // storing remainder in octal array
        octalNum[i] = n % 8;
        n = n / 8;
        i++;
    }
  
    int res =octalNum[0]+octalNum[1]*10+octalNum[2]*100;

    return res;
}

void rwx(int num){

    switch (num) {  /* Print file type */
    case 0:  printf("---"); break;
    case 1:  printf("--x"); break;
    case 2:  printf("-w-"); break;
    case 3:  printf("-wx"); break;
    case 4:  printf("r--"); break;
    case 5:  printf("r-x"); break;
    case 6:  printf("rw-"); break;
    case 7:  printf("rwx"); break;
    default: printf("%d", num); break; /* Should never happen (on Linux) */
    }
}


void per(int permission){

    int owner = permission/100;
    permission = permission - owner*100;
    int group = permission/10;
    int other = permission - group*10;

    rwx(owner);
    rwx(group);
    rwx(other);
}

static int dirTree(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
{
    if (nftw(argv[1], fathers, 10, flags) == -1) {
        perror("nftw");
        exit(EXIT_FAILURE);
    }

    printf(" %*s ├── [", 4 * ftwb->level, " "); 

    if (type == FTW_NS) {                  /* Could not stat() file */
        printf("?");
    } else {
        switch (sbuf->st_mode & S_IFMT) {  /* Print file type */
        case S_IFREG:  
            printf("-");
            per(decToOctal(sbuf->st_mode&0777));
            break;
        case S_IFDIR:  
            printf("d");
            per(decToOctal(sbuf->st_mode&0777));
            break;
        case S_IFCHR:  
            printf("c");
            per(decToOctal(sbuf->st_mode&0777));
            break;
        case S_IFBLK:  
            printf("b");
            per(decToOctal(sbuf->st_mode&0777));
            break;
        case S_IFLNK:  
            printf("l");
            per(decToOctal(sbuf->st_mode&0777));
            break;
        case S_IFIFO:  
            printf("p");
            per(decToOctal(sbuf->st_mode&0777));
            break;
        case S_IFSOCK:
            printf("s");
            per(decToOctal(sbuf->st_mode&0777));
            break;
        default:       printf("?"); break; /* Should never happen (on Linux) */
        }
    }

    struct group *grp;
    struct passwd *pwd;

    pwd = getpwuid(sbuf->st_gid);
    printf(" %s", pwd->pw_name);

    grp = getgrgid(sbuf->st_uid);
    printf("    %s ", grp->gr_name);
        
    off_t size = sbuf->st_size;
    printf("    %ld]  ", size);

    printf("%s\n",  &pathname[ftwb->base]);     /* Print basename */
    return 0;                                   /* Tell nftw() to continue */
}

int
main(int argc, char *argv[])
{
    int flags = 0;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s directory-path\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (nftw(argv[1], dirTree, 10, flags) == -1) {
        perror("nftw");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
