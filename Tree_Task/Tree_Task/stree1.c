#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>

#define BLUE  "\x1b[34m"
#define RESET "\x1b[0m"

typedef struct counter {
  size_t dirs;
  size_t files;
} counter_t;

typedef struct entry {
  char *name;
  char user[100];
  char group[100];
  char *per;
  char type;
  off_t size;
  int is_dir;
  struct entry *next;
} entry_t;

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

char* rwx(int num){

    switch (num) {  /* Print file type */
    case 0:  return "---"; break;
    case 1:  return "--x"; break;
    case 2:  return "-w-"; break;
    case 3:  return "-wx"; break;
    case 4:  return "r--"; break;
    case 5:  return "r-x"; break;
    case 6:  return "rw-"; break;
    case 7:  return "rwx"; break;
    default: return num+""; break; /* Should never happen (on Linux) */
    }
}

void per(int permission, char ans[10]){

    int owner = permission/100;
    permission = permission - owner*100;
    int group = permission/10;
    int other = permission - group*10;

    char* u = rwx(owner);
    char* g = rwx(group);
    char* o = rwx(other);

    for(int i=0; i<3; i++){
        ans[i] = u[i];
    }
    for(int i=3; i<6; i++){
        ans[i] = g[i-3];
    }    
    for(int i=6; i<9; i++){
        ans[i] = o[i-6];
    }
    ans[9] = '\0';
}

int walk(const char* directory, const char* prefix, counter_t *counter) {
  entry_t *head = NULL, *current, *iter;
  size_t size_ = 0, index;

  struct dirent *file_dirent;
  DIR *dir_handle;

  char *full_path, *segment, *pointer, *next_prefix;

  dir_handle = opendir(directory);
  if (!dir_handle) {
    fprintf(stderr, "Cannot open directory \"%s\"\n", directory);
    return -1;
  }

  counter->dirs++;

  while ((file_dirent = readdir(dir_handle)) != NULL) {
    if (file_dirent->d_name[0] == '.') {
      continue;
    }

    current = malloc(sizeof(entry_t));
    current->name = strcpy(malloc(strlen(file_dirent->d_name) + 1), file_dirent->d_name);
    current->is_dir = file_dirent->d_type == DT_DIR;
    current->next = NULL;

    struct stat sbuf;
    
    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s/%s", directory, file_dirent->d_name);
    
    if(stat(full_path, &sbuf)== -1){
        perror("stat");
        return 0;
    }


    char ans[10] = {};
    per(decToOctal(sbuf.st_mode&0777), ans);
    current->per = strdup(ans);


    switch (sbuf.st_mode & S_IFMT) {  /* Print file type */
        case S_IFREG:
            current->type = '-';  
            break;
        case S_IFDIR: 
            current->type = 'd';  
            break;
        case S_IFCHR:
            current->type = 'c';  
            break;
        case S_IFBLK:  
            current->type = 'b';  
            break;
        case S_IFLNK: 
            current->type = 'l';  
            break;
        case S_IFIFO:  
            current->type = 'p';
            break;
        case S_IFSOCK:
            current->type = 's';
            break;
        default:       printf("?"); break; /* Should never happen (on Linux) */
        }

    struct group *grp;
    struct passwd *pwd;

    pwd = getpwuid(sbuf.st_uid);
    strncpy(current->user, pwd->pw_name, sizeof(current->user));
    current->user[sizeof(current->user) - 1] = '\0';

    grp = getgrgid(sbuf.st_gid);
    strncpy(current->group, grp->gr_name, sizeof(current->group));
    current->group[sizeof(current->group) - 1] = '\0';

    off_t size = sbuf.st_size;
    current->size = size;


    if (head == NULL) {
      head = current;
    } else if (strcmp(current->name, head->name) < 0) {
        current->next = head;
        head = current;
    } else {
      for (iter = head; iter->next && strcmp(current->name, iter->next->name) > 0; iter = iter->next);

      current->next = iter->next;
      iter->next = current;
    }

    size_++;
  }

  closedir(dir_handle);
  if (!head) {    
    return 0;
  }

  for (index = 0; index < size_; index++) {
    if (index == size_ - 1) {
      pointer = "└── ";
      segment = "    ";
    } else {
      pointer = "├── ";
      segment = "│   ";
    }

    if(head->type == 'd'){
        printf("%s%s[%c%s %s   %s         %ld] "BLUE"%s\n"RESET, prefix, pointer, head->type, head->per, head->user, head->group, head->size, head->name);
    }
    else{
        printf("%s%s[%c%s %s   %s         %ld] %s\n", prefix, pointer, head->type, head->per, head->user, head->group, head->size, head->name);
    }

    if (head->is_dir) {
      full_path = malloc(strlen(directory) + strlen(head->name) + 2);
      sprintf(full_path, "%s/%s", directory, head->name);

      next_prefix = malloc(strlen(prefix) + strlen(segment) + 1);
      sprintf(next_prefix, "%s%s", prefix, segment);

      walk(full_path, next_prefix, counter);
      free(full_path);
      free(next_prefix);
    } else {
      counter->files++;
    }

    current = head;
    head = head->next;

    free(current->name);
    free(current);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  char* directory = argc > 1 ? argv[1] : ".";
  printf("%s\n", directory);

  counter_t counter = {0, 0};
  walk(directory, "", &counter);

  printf("\n%zu directories, %zu files\n",
    counter.dirs ? counter.dirs - 1 : 0, counter.files);
  return 0;
}