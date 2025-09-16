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

// �û�
typedef struct {
    Key id; // ѧ����
    char passwordHash[crypto_pwhash_STRBYTES]; // �����ϣ
    char name[DATA_NAME_LENGTH + 1]; // ����
    char phoneNumber[DATA_PHONE_NUMBER_LENGTH + 1]; // �ֻ���
    Gender gender; // �Ա�
    UserType userType; // �û�����
    union {
        // ����Ա
        struct { };
        // ��ʦ
        struct {
            char academicTitle[DATA_ACADEMIC_TITLE_LENGTH + 1]; // ְ��
            char office[DATA_OFFICE_LENGTH + 1]; // �칫��
        };
        // ѧ��
        struct {
            int enrollmentYear; // ��ѧ���
            char placeOfOrigin[DATA_PLACE_OF_ORIGIN_LENGTH + 1]; // ��Դ��
        };
    };
} User;

// �γ�
typedef struct {
    Key id; // �γ̱�ţ�8λ��
    char name[DATA_COURSE_NAME_LENGTH + 1]; // �γ���
    int credit; // ѧ�ֵ�ʮ��
    Key teacherId; // �ον�ʦ����
} Course;

// �������ݿ�ԭ��ε�˵�������� Course �� Section Ӧ���ֿ�
// Course ����һ�ſΣ�Section �������ſο���ĳһ����
// ���Ǽ��������Ͳ������ˣ������߼���Ҳ˵��ͨ

// �δ�
typedef struct {
    Key courseId; // �γ̱��
    Key studentId; // ѧ��ѧ��
    int score; // �÷�
} Take;

// ѧϰ����
typedef struct {
    Key id; // ���ϱ��
    Key courseId; // �γ̱��
    char title[DATA_MATERIAL_TITLE_LENGTH + 1]; // ��������
    char path[DATA_MATERIAL_PATH_LENGTH + 1]; // ����·��
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