#ifndef DATA_H
#define DATA_H

#include <sodium.h>
#include "bPlusTree.h"

#define DATA_NAME_LENGTH 20
#define DATA_COURSE_NAME_LENGTH 60

typedef enum { USER_ADMIN, USER_TEACHER, USER_STUDENT } UserType;

typedef struct {
    Key id;
    char passwordHash[crypto_pwhash_STRBYTES];
    UserType userType;
    union {
        // 管理员
        struct { };
        // 老师
        struct { };
        // 学生
        struct {
            short grade;
        };
    };
} User;

typedef struct {
    Key id;
    char name[DATA_COURSE_NAME_LENGTH + 1];
} Course;

#endif