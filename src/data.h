#ifndef DATA_H
#define DATA_H

#include <sodium.h>
#include "bPlusTree.h"

#define DATA_NAME_LENGTH 20
#define DATA_PASSWORD_LENGTH 20
#define DATA_PHONE_NUMBER_LENGTH 13
#define DATA_ACADEMIC_TITLE_LENGTH 6
#define DATA_OFFICE_LENGTH 20
#define DATA_PLACE_OF_ORIGIN_LENGTH 6

#define DATA_COURSE_NAME_LENGTH 60

#define DATA_MATERIAL_TITLE_LENGTH 60
#define DATA_MATERIAL_PATH_LENGTH 100

typedef enum { USER_ADMIN, USER_TEACHER, USER_STUDENT } UserType;
typedef enum { GENDER_FEMALE, GENDER_MALE } Gender;

// 用户
typedef struct {
    Key id; // 学工号
    char passwordHash[crypto_pwhash_STRBYTES]; // 密码哈希
    char name[DATA_NAME_LENGTH + 1]; // 姓名
    char phoneNumber[DATA_PHONE_NUMBER_LENGTH + 1]; // 手机号
    Gender gender; // 性别
    UserType userType; // 用户类型
    union {
        // 管理员
        struct { };
        // 老师
        struct {
            char academicTitle[DATA_ACADEMIC_TITLE_LENGTH + 1]; // 职称
            char office[DATA_OFFICE_LENGTH + 1]; // 办公室
        };
        // 学生
        struct {
            int enrollmentYear; // 入学年份
            char placeOfOrigin[DATA_PLACE_OF_ORIGIN_LENGTH + 1]; // 生源地
        };
    };
} User;

// 课程
typedef struct {
    Key id; // 课程编号（8位）
    char name[DATA_COURSE_NAME_LENGTH + 1]; // 课程名
    int credit; // 学分的十倍
    Key teacherId; // 任课教师工号
} Course;

// 按照数据库原理课的说法，这里 Course 和 Section 应当分开
// Course 代表一门课，Section 代表这门课开的某一个班
// 但是简化起见这里就不区分了，反正逻辑上也说得通

// 课次
typedef struct {
    Key courseId; // 课程编号
    Key studentId; // 学生学号
    int score; // 得分
} Take;

// 学习资料
typedef struct {
    Key id; // 资料编号
    Key courseId; // 课程编号
    char title[DATA_MATERIAL_TITLE_LENGTH + 1]; // 资料名称
    char path[DATA_MATERIAL_PATH_LENGTH + 1]; // 资料路径
} Material;

void saveData(char* filename, BPTree* tree, size_t size);
int readData(char* filename, size_t size, void(*operation)(void*));

int verifyPassword(char* password, char* hash);
int setPassword(char* password, char* hash);

#define DECLARE_DATA_FUNC(Type, withInsertFunc) \
    DECLARE_B_PLUS_TREE_FUNC(Type) \
    static inline void saveData##Type(char* filename, BPTree* tree) { saveData(filename, tree, sizeof(Type)); } \
    static inline int readData##Type(char* filename, void(*operation)(Type*)) { return readData(filename, sizeof(Type), (void (*)(void*))operation); } \
    DECLARE_DATA_INSERT_FUNC_##withInsertFunc(Type)

#define DECLARE_DATA_INSERT_FUNC_true(Type) \
    static inline void insert##Type##RecordById(Type* record) { insertRecord(Type##sIndexId, record->id, record); }

#define DECLARE_DATA_INSERT_FUNC_false(Type)

#endif