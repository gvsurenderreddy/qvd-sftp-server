/* Qindel */
#include "../qvd_includes.h"
#include <stdlib.h>

static struct passwd qvd_pwd = {
	"qvd",    /* username */
	"secret", /* password */
	0,        /* uid */
	0,        /* gid */
	"QVD",    /* full name */
	NULL,     /* home dir */
	""        /* shell */
};
	
struct passwd *
getpwuid(uid_t uid) {
	qvd_pwd.pw_dir = getenv("USERPROFILE");
	return &qvd_pwd;
}

struct passwd *
getpwnam(const char *name) {
	return getpwuid(0);
}

char *user_from_uid(uid_t uid, int nouser) {
	struct passwd *pwd = getpwuid(uid);
	return pwd->pw_name;
}

char *
group_from_gid(gid_t gid, int nogroup) {
	return "qvd";
}

int 
getuid() {
	return 0;
}