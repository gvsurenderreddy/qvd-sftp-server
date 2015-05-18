#ifndef _QVD_PW_H
#define _QVD_PW_H


struct passwd *getpwuid(uid_t uid);

struct passwd *getpwnam(const char *name);

char *user_from_uid(uid_t uid, int nouser);
char *group_from_gid(gid_t gid, int nogroup);

int getuid();

#endif