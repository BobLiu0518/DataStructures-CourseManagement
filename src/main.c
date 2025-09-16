#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <string.h>
#include <signal.h>
#include <shellapi.h>
#include "debug.h"
#include "033.h"
#include "elegantDisplay.h"
#include "bPlusTree.h"
#include "data.h"

// ����ȫ�ֱ������������� ���� + Index + ���� ����ʽ
// compound ��ʾ���������������������������
BPTree* UsersIndexId;
BPTree* CoursesIndexId, * CoursesIndexTeacherId;
BPTree* TakesIndexCompound, * TakesIndexCourseId;
BPTree* MaterialsIndexId, * MaterialsIndexCourseId;

// ��үһ�����£��ܼҾ������� 5 ����������������ȫ��
DECLARE_DATA_FUNC(User, true);
DECLARE_DATA_FUNC(Course, false);
DECLARE_DATA_FUNC(Take, false);
DECLARE_DATA_FUNC(Material, false);

void insertCourseRecord(Course* course) {
    insertRecord(CoursesIndexId, course->id, course);
    insertRecord(CoursesIndexTeacherId, course->teacherId, course);
}

void insertTakeRecord(Take* take) {
    insertRecord(TakesIndexCompound, take->studentId * 100000000ULL + take->courseId, take);
    insertRecord(TakesIndexCourseId, take->courseId, take);
}

void insertMaterialRecord(Material* material) {
    insertRecord(MaterialsIndexId, material->id, material);
    insertRecord(MaterialsIndexCourseId, material->courseId, material);
}

void addCourse() {
    displayTitle("����ϵͳ - ��ӿγ�");
    Course* course = malloc(sizeof(Course));
    memset(course, 0, sizeof(Course));

    displayInput("�γ̱��", "%llu", &course->id);
    if (!course->id) {
        printf(Red("ʧ�ܣ�")"��������γ̱�š�");
        free(course);
        pause();
        return;
    } else if (findRecord(CoursesIndexId, course->id)) {
        printf(Red("ʧ�ܣ�")"�γ��Ѵ��ڡ�");
        free(course);
        pause();
        return;
    }
    displayInput("��ʦ����", "%llu", &course->teacherId);
    User* teacher = findRecord(UsersIndexId, course->teacherId);
    if (!teacher || teacher->userType != USER_TEACHER) {
        printf(Red("ʧ�ܣ�")"��ʦ�����ڡ�");
        free(course);
        pause();
        return;
    }
    displayInput("�γ�����", "%[^\n]", course->name);
    double credit;
    displayInput("�γ�ѧ��", "%lf", &credit);
    course->credit = credit * 10;
    displayInput("�Ͽ�ʱ��", "%[^\n]", course->time);
    displayInput("�Ͽν���", "%[^\n]", course->classroom);

    insertRecord(CoursesIndexId, course->id, course);
    insertRecord(CoursesIndexTeacherId, course->teacherId, course);
    printf(Green("��ӿγ� %s �ɹ���"), course->name);
    pause();
}

Course* selectCourse(RecordArray courses) {
    int currentPage = 0, totalPages;
    char pageInfo[41], * options[9];
    Course* selection = nullptr;

    if (!courses.total) {
        displayTitle("����ϵͳ - ѡ��γ�");
        printf("�޿�ѡ�γ�");
        pause();
        return nullptr;
    }

    for (int i = 0; i < 8; i++) {
        options[i] = calloc(41, sizeof(char));
    }
    if (!courses.total) {
        totalPages = 0;
    } else if (courses.total <= 9) {
        totalPages = 1;
    } else {
        totalPages = (courses.total - 16) / 7 + 2;
    }

    while (true) {
        bool isFirstPage = currentPage == 0, isLastPage = currentPage == totalPages - 1;
        int count = 0;
        sprintf(pageInfo, "�� %d ҳ / �� %d ҳ", currentPage + 1, totalPages);
        if (!isFirstPage) {
            strcpy(options[count++], "(��һҳ)");
        }
        while (count < (isLastPage ? 9 : 8) && currentPage * 7 + !isFirstPage + count < courses.total) {
            strcpy(options[count], ((Course*)courses.arr[currentPage * 7 + !isFirstPage + count])->name);
            count++;
        }
        if (!isLastPage) {
            strcpy(options[count++], "(��һҳ)");
        }
        int choice = displaySelect("����ϵͳ - ѡ��γ�", pageInfo, count, options);
        if (choice == -1) {
            break;
        } else if (!isFirstPage && choice == 0) {
            currentPage--;
        } else if (!isLastPage && choice == count - 1) {
            currentPage++;
        } else {
            selection = courses.arr[currentPage * 7 + !isFirstPage + choice];
            break;
        }
    }

    free(courses.arr);
    for (int i = 0; i < 8; i++) {
        free(options[i]);
    }
    return selection;
}

void showCourseDetail(Course* course, bool withId) {
    if (withId) {
        printf("�γ̱��: "BlackBright("%llu\n"), course->id);
    }
    printf("�γ�����: "BlackBright("%s\n"), course->name);
    printf("�γ�ѧ��: "BlackBright("%.1lf\n"), course->credit / 10.0);
    printf("�Ͽ�ʱ��: "BlackBright("%s\n"), course->time);
    printf("�Ͽν���: "BlackBright("%s\n"), course->classroom);

    User* teacher = findRecord(UsersIndexId, course->teacherId);
    if (!teacher) {
        printf("�ον�ʦ: "BlackBright("δ֪\n"));
    } else {
        printf("�ον�ʦ: "BlackBright("%s[%s] (%llu)\n"), teacher->name, teacher->academicTitle, teacher->id);
    }
    pause();
}

void listCourses() {
    Course* course = selectCourse(findRecordRangeArray(CoursesIndexId, 0, UINT64_MAX));
    if (course) {
        displayTitle("����ϵͳ - �γ���Ϣ");
        showCourseDetail(course, true);
    }
}

void queryCourse() {
    displayTitle("����ϵͳ - ��ѯ�γ�");
    Key courseId;

    displayInput("�γ̱��", "%llu", &courseId);
    Course* course = findRecord(CoursesIndexId, courseId);
    if (!course) {
        printf(Yellow("δ�ҵ��γ� %llu��"), courseId);
        pause();
        return;
    }

    showCourseDetail(course, false);
}


void statScore(Course* course, bool showTeacher) {
    displayTitle("����ϵͳ - �ɼ�ͳ��");
    RecordArray takes = findRecordRangeArray(TakesIndexCourseId, course->id, course->id);
    int setCount = takes.total, failedCount = 0, passedCount = 0, excellentCount = 0, totalScore = 0, highestScore = -1;
    char highestOwner[100];
    for (int i = 0; i < takes.total; i++) {
        Take* take = takes.arr[i];
        if (take->score == DATA_SCORE_NOT_SET) {
            setCount--;
            continue;
        }

        totalScore += take->score;
        if (take->score >= highestScore) {
            User* student = findRecord(UsersIndexId, take->studentId);
            if (take->score > highestScore) {
                highestScore = take->score;
                strcpy(highestOwner, student->name);
            } else {
                sprintf_s(highestOwner, 100, "%s %s", highestOwner, student->name);
            }
        }
        if (take->score < 600) {
            failedCount++;
        } else if (take->score < 900) {
            passedCount++;
        } else {
            excellentCount++;
        }
    }
    free(takes.arr);

    printf("�γ�: "BlackBright("%s\n"), course->name);
    if (showTeacher) {
        User* teacher = findRecord(UsersIndexId, course->teacherId);
        printf("��ʦ: "BlackBright("%s [%s] (%llu)\n"), teacher->name, teacher->academicTitle, teacher->id);
    }
    printf("����ѧ�� %2d �������гɼ�δ¼�� %2d ��\n", takes.total, takes.total - setCount);
    if (setCount) {
        printf("��¼��ɼ���ѧ���У�\n");
        printf("������ %2d �� (%5.2lf%%)\n", excellentCount, excellentCount * 100.0 / setCount);
        printf("���ϸ� %2d �� (%5.2lf%%)\n", passedCount, passedCount * 100.0 / setCount);
        printf("���ϸ� %2d �� (%5.2lf%%)\n", failedCount, failedCount * 100.0 / setCount);
        printf("�༶ƽ���� %5.2lf ��\n", totalScore * 0.1 / setCount);
        printf("��߷� %4.1lf �֣�%s\n", highestScore / 10.0, highestOwner);
    }
    pause();
}

void statCourse() {
    Course* course = selectCourse(findRecordRangeArray(CoursesIndexId, 0, UINT64_MAX));
    if (course) {
        statScore(course, true);
    }
}

void modifyCourse() {
    displayTitle("����ϵͳ - �޸Ŀγ�");
    Key courseId;

    displayInput("�γ̱��", "%llu", &courseId);
    Course* course = findRecord(CoursesIndexId, courseId);
    if (!course) {
        printf(Red("ʧ�ܣ�")"�γ̲����ڡ�");
        pause();
        return;
    }

    while (true) {
        char courseInfo[100];
        sprintf(courseInfo, "%s (%llu)", course->name, course->id);
        int choice = displaySelect("����ϵͳ - �޸Ŀγ�", courseInfo, OPTIONS("�޸�����", "�޸�ѧ��", "�޸Ľ�ʦ", "�˳��޸�"));
        displayTitle("����ϵͳ - �޸Ŀγ�");
        switch (choice) {
        case 0:
            displayInput("�γ�����", "%[^\n]", course->name);
            break;
        case 1:
            double credit;
            displayInput("�γ�ѧ��", "%lf", &credit);
            course->credit = credit * 10;
            break;
        case 2:
            Key teacherId;
            displayInput("��ʦ����", "%llu", &teacherId);
            User* teacher = findRecord(UsersIndexId, teacherId);
            if (!teacher || teacher->userType != USER_TEACHER) {
                printf(Red("ʧ�ܣ�")"��ʦ�����ڡ�");
                pause();
                continue;
            }
            removeRecord(CoursesIndexTeacherId, course->teacherId, course);
            course->teacherId = teacherId;
            insertRecord(CoursesIndexTeacherId, course->teacherId, course);
            break;
        default:
            return;
        }
        printf(Green("�޸ĳɹ���"));
        pause();
    }
}

void deleteCourse() {
    displayTitle("����ϵͳ - ɾ���γ�");
    Key courseId;

    displayInput("�γ̱��", "%llu", &courseId);
    Course* course = findRecord(CoursesIndexId, courseId);
    if (!course) {
        printf(Red("ʧ�ܣ�")"�γ̲����ڡ�");
        pause();
        return;
    }
    char prompt[100];
    sprintf(prompt, "ȷ��Ҫɾ���γ� %s ��", course->name);
    if (displaySelect("����ϵͳ - ɾ���γ�", prompt, OPTIONS("��", "��")) != 1) {
        return;
    }

    displayTitle("����ϵͳ - ɾ���γ�");
    removeRecord(CoursesIndexId, courseId, nullptr);
    removeRecord(CoursesIndexTeacherId, course->teacherId, course);
    free(course);
    printf(Green("ɾ���γ̳ɹ���"));
    pause();

    // ����������ô���ģ�����ͻȻ�뵽һ�߱���һ��ɾ���������⣬����ʱ�����Ż���
    // Edit: ���ڿ����� findRecordRangeArray ʵ���ˣ��Ȼ�����ɾ
    // TODO
    // if (displaySelect("����ϵͳ - ɾ���γ�", "�Ƿ�Ҫɾ���ÿγ̵�������ݣ�", OPTIONS("��", "��")) != 0) {
    //     return;
    // }
    // 
    // findRecordRange(TakesIndexCourseId, courseId, courseId, removeTake); // ���ԣ�
}

int getGradePoint(int score) {
    if (score < 600) {
        return 0;
    } else if (score < 650) {
        return 10;
    } else if (score < 700) {
        return 15;
    } else if (score < 750) {
        return 20;
    } else if (score < 800) {
        return 25;
    } else if (score < 850) {
        return 30;
    } else if (score < 900) {
        return 35;
    } else if (score < 950) {
        return 40;
    } else {
        return 45;
    }
}

void queryGPA(User* student) {
    displayTitle("����ϵͳ - ��ѯƽ������");

    if (!student) {
        Key studentId;
        displayInput("ѧ��ѧ��", "%llu", &studentId);
        student = findRecord(UsersIndexId, studentId);
        if (!student) {
            printf(Red("����")"ѧ�������ڡ�");
            pause();
            return;
        }
    }

    RecordArray takes = findRecordRangeArray(TakesIndexCompound, student->id * 100000000ULL, (student->id + 1) * 100000000ULL - 1);
    int totalCredit = 0, totalCreditGradePoint = 0;
    for (int i = 0; i < takes.total; i++) {
        Take* take = takes.arr[i];
        Course* course = findRecord(CoursesIndexId, take->courseId);
        totalCredit += course->credit;
        totalCreditGradePoint += course->credit * getGradePoint(take->score);
    }
    printf("ѧ��: "BlackBright("%s\n"), student->name);
    printf("���޵���ѧ��: "BlackBright("%.1lf\n"), totalCredit / 10.0);
    if (takes.total) {
        printf("ƽ��ѧ�ּ���: "BlackBright("%.2lf\n"), totalCreditGradePoint / 10.0 / totalCredit);
    }

    free(takes.arr);
    pause();
}

Material* selectMaterial(RecordArray materials) {
    char** options = calloc(materials.total, sizeof(char*));

    if (!materials.total) {
        displayTitle("����ϵͳ - ѡ��ѧϰ����");
        printf("�޿�ѡ����");
        pause();
        return nullptr;
    }

    for (int i = 0; i < materials.total; i++) {
        options[i] = ((Material*)materials.arr[i])->title;
    }

    int selection = displaySelect("����ϵͳ - ѡ��ѧϰ����", nullptr, materials.total, options);
    Material* material = selection != -1 ? materials.arr[selection] : nullptr;

    free(options);
    free(materials.arr);
    return material;
}

void listAllMaterials() {
    Material* material = selectMaterial(findRecordRangeArray(MaterialsIndexId, 0, UINT64_MAX));
    if (material) {
        displayTitle("����ϵͳ - �鿴ѧϰ����");
        printf("���ڲ鿴ѧϰ���ϡ�\n");
        system(material->path);
    }
}

void statAvgSelected() {
    displayTitle("����ϵͳ - ͳ��ƽ��ѡ��");
    Key teacherId;
    displayInput("��ʦ����", "%llu", &teacherId);
    User* teacher = findRecord(UsersIndexId, teacherId);
    if (!teacher) {
        printf(Red("����")"��ʦ�����ڡ�");
        pause();
        return;
    }

    RecordArray courses = findRecordRangeArray(CoursesIndexTeacherId, teacher->id, teacher->id);
    int totalStudents = 0;
    for (int i = 0; i < courses.total; i++) {
        Course* course = courses.arr[i];
        RecordArray takes = findRecordRangeArray(TakesIndexCourseId, course->id, course->id);
        totalStudents += takes.total;
        free(takes.arr);
    }
    free(courses.arr);

    printf("��ʦ: "BlackBright("%s [%s] (%llu)\n"), teacher->name, teacher->academicTitle, teacher->id);
    printf("�ܹ����� %d �ſγ�\n", courses.total);
    if (courses.total) {
        printf("ƽ��ѡ������Ϊ %.1lf ��\n", totalStudents * 1.0 / courses.total);
    }
    pause();
}

void adminMenu(User* user) {
    char welcome[41];
    sprintf(welcome, "��ӭ��%s��", user->name);
    while (true) {
        switch (displaySelect("����ϵͳ - ����Ա", welcome, OPTIONS(
            "��ӿγ�",
            "����γ�",
            "��ѯ�γ�",
            "�޸Ŀγ�",
            "ɾ���γ�",
            "��ѯѧ��ƽ������",
            "�鿴ȫ��ѧϰ����",
            "ͳ�ƿγ̳ɼ����",
            "ͳ�ƽ�ʦƽ��ѡ������",
            "�˳���¼"
        ))) {
        case 0:
            addCourse();
            break;
        case 1:
            listCourses();
            break;
        case 2:
            queryCourse();
            break;
        case 3:
            modifyCourse();
            break;
        case 4:
            deleteCourse();
            break;
        case 5:
            queryGPA(nullptr);
            break;
        case 6:
            listAllMaterials();
            break;
        case 7:
            statCourse();
            break;
        case 8:
            statAvgSelected();
            break;
        default:
            return;
        }
    }
}

void addMaterial(Course* course) {
    char filepath[DATA_MATERIAL_PATH_LENGTH + 1] = "storage\\materials\\", title[DATA_MATERIAL_TITLE_LENGTH + 1];
    displayTitle("����ϵͳ - ���ѧϰ����");
    printf("�뽫ѧϰ���Ϸ�����\nstorage\\materials �ļ����¡�\n");
    displayInput("�ļ���", "%[^\n]", filepath + strlen(filepath));

    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        printf(Red("ʧ�ܣ�")"�ļ������ڡ�\n");
        pause();
        return;
    }
    fclose(fp);
    displayInput("����", "%[^\n]", title);

    Material* material = malloc(sizeof(Material));
    material->id = getBiggestKey(MaterialsIndexId) + 1;
    material->courseId = course->id;
    strcpy(material->title, title);
    strcpy(material->path, filepath);

    insertMaterialRecord(material);
    printf(Green("���ѧϰ���ϳɹ���"));
    pause();
}

void showMaterial(Course* course) {
    Material* material = selectMaterial(findRecordRangeArray(MaterialsIndexCourseId, course->id, course->id));
    if (material) {
        displayTitle("����ϵͳ - �鿴ѧϰ����");
        printf("���ڲ鿴ѧϰ���ϡ�\n");
        system(material->path);
    }
}

void modifyMaterial(Course* course) {
    Material* material = selectMaterial(findRecordRangeArray(MaterialsIndexCourseId, course->id, course->id));
    if (!material) {
        return;
    }

    while (true) {
        int choice = displaySelect("����ϵͳ - �޸�ѧϰ����", material->title, OPTIONS(
            "�޸����ϱ���",
            "�޸������ļ�",
            "�˳��޸�"
        ));
        displayTitle("����ϵͳ - �޸�ѧϰ����");
        switch (choice) {
        case 0:
            displayInput("����", "%[^\n]", material->title);
            break;
        case 1:
            char filepath[DATA_MATERIAL_PATH_LENGTH + 1] = "storage\\materials\\";
            printf("�뽫ѧϰ���Ϸ�����\nstorage\\materials �ļ����¡�\n");
            displayInput("�ļ���", "%[^\n]", filepath + strlen(filepath));

            FILE* fp = fopen(filepath, "r");
            if (!fp) {
                printf(Red("����")"�ļ�%s�����ڡ�", filepath);
                pause();
                continue;
            }
            fclose(fp);
            break;
        default:
            return;
        }
        printf(Green("�޸ĳɹ���"));
        pause();
    }
}

void deleteMaterial(Course* course) {
    Material* material = selectMaterial(findRecordRangeArray(MaterialsIndexCourseId, course->id, course->id));
    if (!material) {
        return;
    }

    char prompt[100];
    sprintf(prompt, "ȷ��Ҫɾ������ %s ��", material->title);
    if (displaySelect("����ϵͳ - ɾ��ѧϰ����", prompt, OPTIONS("��", "��")) != 1) {
        return;
    }

    displayTitle("����ϵͳ - ɾ��ѧϰ����");
    DeleteFile(material->path);
    removeRecord(MaterialsIndexId, material->id, nullptr);
    removeRecord(MaterialsIndexCourseId, material->courseId, material);
    free(material);
    printf(Green("ɾ��ѧϰ���ϳɹ���"));
    pause();
}

void modifyScore(Course* course) {
    displayTitle("����ϵͳ - ¼��ɼ�");
    Key studentId;
    displayInput("ѧ��ѧ��", "%llu", &studentId);
    User* student = findRecord(UsersIndexId, studentId);
    if (!student || student->userType != USER_STUDENT) {
        printf(Red("����")"δ�ҵ�ѧ����");
        pause();
        return;
    }

    Take* take = findRecord(TakesIndexCompound, student->id * 100000000ULL + course->id);
    if (!take) {
        printf(Yellow("ѧ�� %s δѡ�γ� %s��"), student->name, course->name);
        pause();
        return;
    }

    printf("ѧ��: "BlackBright("%s (%llu)\n"), student->name, student->id);
    printf("�γ�: "BlackBright("%s (%llu)\n"), course->name, course->id);
    if (take->score != DATA_SCORE_NOT_SET) {
        printf("�ɼ�: "BlackBright("%.1lf\n"), take->score / 10.0);
        pause();
        if (displaySelect("����ϵͳ - ¼��ɼ�", "�ɼ��Ѵ��ڣ��Ƿ�����¼�룿", OPTIONS("��", "��")) != 1) {
            return;
        }
        displayTitle("����ϵͳ - ¼��ɼ�");
        printf("ѧ��: "BlackBright("%s (%llu)\n"), student->name, student->id);
        printf("�γ�: "BlackBright("%s (%llu)\n"), course->name, course->id);
    }

    double score;
    displayInput("�ɼ�", "%lf", &score);
    if (score < 0 && score > 100) {
        printf(Red("����")"�ɼ����Ϸ���");
        pause();
        return;
    }
    take->score = score * 10;
    printf(Green("¼��ɼ��ɹ���"));
    pause();
}

void studentList(Course* course) {
    displayTitle("����ϵͳ - ѧ������");
    RecordArray takes = findRecordRangeArray(TakesIndexCourseId, course->id, course->id);
    for (int i = 0; i < takes.total; i++) {
        Take* take = takes.arr[i];
        User* student = findRecord(UsersIndexId, take->studentId);
        printf("%s (%llu)\n", student->name, student->id);
    }
    free(takes.arr);
    pause();
}

void manageCourse(Course* course) {
    while (true) {
        switch (displaySelect("����ϵͳ - �γ̹���", course->name, OPTIONS(
            "�γ���Ϣ",
            "���ѧϰ����",
            "�鿴ѧϰ����",
            "�޸�ѧϰ����",
            "ɾ��ѧϰ����",
            "ѧ������",
            "ͳ�Ƴɼ�",
            "¼��ɼ�",
            "�˳�����"
        ))) {
        case 0:
            displayTitle("����ϵͳ - �γ���Ϣ");
            showCourseDetail(course, true);
            break;
        case 1:
            addMaterial(course);
            break;
        case 2:
            showMaterial(course);
            break;
        case 3:
            modifyMaterial(course);
            break;
        case 4:
            deleteMaterial(course);
            break;
        case 5:
            studentList(course);
            break;
        case 6:
            statScore(course, false);
            break;
        case 7:
            modifyScore(course);
            break;
        default:
            return;
        }
    }
}

void teacherMenu(User* user) {
    char welcome[41];
    sprintf(welcome, "��ӭ��%s��", user->name);
    while (true) {
        switch (displaySelect("����ϵͳ - ��ʦ", welcome, OPTIONS(
            "�γ̹���",
            "�˳���¼"
        ))) {
        case 0:
            Course * course = selectCourse(findRecordRangeArray(CoursesIndexTeacherId, user->id, user->id));
            if (course) {
                manageCourse(course);
            }
            break;
        default:
            return;
        }
    }
}

RecordArray takenCourses(RecordArray takes) {
    for (int i = 0; i < takes.total; i++) {
        takes.arr[i] = findRecord(CoursesIndexId, ((Take*)takes.arr[i])->courseId);
    }
    return takes;
}

void viewCourse(User* user, Course* course) {
    Take* take = findRecord(TakesIndexCompound, user->id * 100000000ULL + course->id);
    if (!take) {
        displayTitle("����ϵͳ - �γ���Ϣ");
        printf("�㻹δѡ�����ſΡ�\n");
        pause();
        return;
    }
    while (true) {
        switch (displaySelect("����ϵͳ - �γ���Ϣ", course->name, OPTIONS(
            "�γ���Ϣ��ѯ",
            "�γ̳ɼ���ѯ",
            "ѧϰ���ϲ�ѯ",
            "�˳��γ���Ϣ"
        ))) {
        case 0:
            displayTitle("����ϵͳ - �γ���Ϣ");
            showCourseDetail(course, true);
            break;
        case 1:
            displayTitle("����ϵͳ - �γ̳ɼ�");
            printf("ѧ��: "BlackBright("%s (%llu)\n"), user->name, user->id);
            printf("�γ�: "BlackBright("%s (%llu)\n"), course->name, course->id);
            if (take->score == DATA_SCORE_NOT_SET) {
                printf(Yellow("�ɼ���δ¼�룬����ϵ�ον�ʦ\n"));
            } else if (take->score < 600) {
                printf(Yellow("�����ɼ� %.1lf�����ϸ�\n"), take->score / 10.0);
            } else if (take->score < 900) {
                printf(Green("�����ɼ� %.1lf���ϸ�\n"), take->score / 10.0);
            } else {
                printf(Green("�����ɼ� %.1lf������\n"), take->score / 10.0);
            }
            pause();
            break;
        case 2:
            showMaterial(course);
            break;
        default:
            return;
        }
    }
}

void studentMenu(User* user) {
    char welcome[41];
    sprintf(welcome, "��ӭ��%s��", user->name);
    while (true) {
        switch (displaySelect("����ϵͳ - ѧ��", welcome, OPTIONS(
            "�γ���Ϣ",
            "��ѯƽ������",
            "�˳���¼"
        ))) {
        case 0:
            Course * course = selectCourse(takenCourses(findRecordRangeArray(TakesIndexCompound, user->id * 100000000ULL, (user->id + 1) * 100000000ULL - 1)));
            if (course) {
                viewCourse(user, course);
            }
            break;
        case 1:
            queryGPA(user);
            break;
        default:
            return;
        }
    }
}

void testBPTree() {
    int count, addCount, removeCount, allowDuplicateKey;
    unsigned seed;
    displayTitle("���� - B+ ������");
    displayInput("ѭ�����Դ���", "%d", &count);
    displayInput("���Բ������", "%d", &addCount);
    displayInput("����ɾ������", "%d", &removeCount);
    displayInput("�Ƿ������ظ�", "%d", &allowDuplicateKey);
    if (count < 0 || removeCount > addCount || addCount > 100) {
        printf(Red("ʧ�ܣ�")"��������");
        pause();
        return;
    }
    displayInput("�������", "%u", &seed);
    if (!seed) {
        seed = time(NULL);
    }
    srand(seed);
    printDebug("ʹ�õ�������ӣ�%u", seed);

    CreateDirectory("storage\\debug", nullptr);
    for (int t = 0; !count || t < count; t++) {
        Key data[addCount];
        char filename[50];
        BPTree* tree = createTree(nullptr, allowDuplicateKey != 0);
        for (int i = 0; i < addCount; i++) {
            data[i] = rand() % 100;
            printDebug("[%03d] ���Բ��� %d", i + 1, data[i]);
            if (insertRecord(tree, data[i], data + i)) {
                sprintf(filename, "storage\\debug\\test%03d.html", i + 1);
                saveTreeMermaid(tree, filename, false);
                checkTreeLegitimacy(tree, true);
            } else {
                printDebug("����ʧ�ܣ��ظ���");
                i--;
            }
        }
        for (int i = addCount; i < addCount + removeCount; i++) {
            int removeIndex = rand() % addCount;
            int removeData = data[removeIndex];
            printDebug("[%03d] ����ɾ�� %d", i + 1, removeData);
            if (removeRecord(tree, removeData, data + removeIndex)) {
                sprintf(filename, "storage\\debug\\test%03d.html", i + 1);
                saveTreeMermaid(tree, filename, false);
                checkTreeLegitimacy(tree, true);
            } else {
                printDebug("ɾ��ʧ�ܣ�δ�ҵ���");
                i--;
            }
        }
        printDebug("�������ٲ�������");
        destroyTree(tree);
    }
    printf(Green("\n��Ү��ȫ����ɣ�"));
    pause();
}

// �����ã�û��������Ĳ�����֤
void addUser() {
    displayTitle("���� - ����û�");
    User* user = malloc(sizeof(User));
    char password[DATA_PASSWORD_LENGTH + 1];

    displayInput("ѧ����", "%llu", &user->id);
    displayInputPassword("��ʼ����", password, DATA_PASSWORD_LENGTH);
    setPassword(password, user->passwordHash);
    displayInput("����", "%s", user->name);
    displayInput("�ֻ���", "%s", user->phoneNumber);
    user->gender = displaySelect("���� - ����û�", "ѡ���Ա�", OPTIONS("Ů", "��"));
    user->userType = displaySelect("���� - ����û�", "ѡ���û�����", OPTIONS("����Ա", "��ʦ", "ѧ��"));

    displayTitle("���� - ����û�");
    switch (user->userType) {
    case USER_ADMIN:
        break;
    case USER_TEACHER:
        displayInput("ְ��", "%s", user->academicTitle);
        displayInput("�칫��", "%[^\n]", user->office);
        break;
    case USER_STUDENT:
        displayInput("�꼶", "%d", &user->enrollmentYear);
        displayInput("��Դ��", "%s", user->placeOfOrigin);
        break;
    }

    insertRecord(UsersIndexId, user->id, user);
    printf(Green("��ӳɹ���"));
    pause();
}

void deleteUser() {
    displayTitle("���� - ɾ���û�");

    Key id;
    displayInput("ѧ����", "%llu", &id);
    if (removeRecord(UsersIndexId, id, nullptr)) {
        printf(Green("ɾ���ɹ���"));
    } else {
        printf(Red("����")"δ�ҵ����û���");
    }
    pause();
}

void debugMenu() {
    while (true) {
        switch (displaySelect("����ϵͳ - ����", nullptr, OPTIONS(
            "���� B+ ��",
            "����û�",
            "ɾ���û�",
            "�˳�"
        ))) {
        case 0:
            testBPTree();
            break;
        case 1:
            addUser();
            break;
        case 2:
            deleteUser();
            break;
        default:
            return;
        }
    }
}

void login() {
    Key username;
    char password[DATA_PASSWORD_LENGTH + 1];
    displayTitle("����ϵͳ - ��¼");
    displayInput("ѧ����", "%llu", &username);
    User* user = findRecord(UsersIndexId, username);
    if (!user) {
        printf(Red("��¼ʧ�ܣ�")"�û������ڡ�");
        pause();
        return;
    }
    displayInputPassword("����", password, DATA_PASSWORD_LENGTH);
    if (verifyPassword(password, user->passwordHash) != 0) {
        printf(Red("��¼ʧ�ܣ�")"�������");
        pause();
        return;
    }
    printf(Green("��ã�%s��"), user->name);
    pause();
    switch (user->userType) {
    case USER_ADMIN:
        adminMenu(user);
        break;
    case USER_TEACHER:
        teacherMenu(user);
        break;
    case USER_STUDENT:
        studentMenu(user);
        break;
    }
}

void resetPassword() {
    Key username;
    char verifyCode[7], phoneNumber[DATA_PHONE_NUMBER_LENGTH + 1], newPassword[DATA_PASSWORD_LENGTH + 1];
    displayTitle("����ϵͳ - ��������");
    displayInput("ѧ����", "%llu", &username);
    User* user = findRecord(UsersIndexId, username);
    if (!user) {
        printf(Red("ʧ�ܣ�")"�û������ڡ�");
        pause();
        return;
    }
    strcpy(phoneNumber, user->phoneNumber);
    for (int i = 3; i < 7; i++) {
        phoneNumber[i] = '*';
    }
    printf("��֤���ѷ����� %s��\n", phoneNumber); // ��û��
    displayInput("��֤��", "%s", verifyCode);
    if (strlen(verifyCode) != 6) {
        printf(Red("ʧ�ܣ�")"��֤�����");
        pause();
        return;
    }
    displayInputPassword("����������", newPassword, DATA_PASSWORD_LENGTH);
    if (strlen(newPassword) < 8) {
        printf(Red("ʧ�ܣ�")"��������Ϊ 8 λ��");
        pause();
        return;
    }
    setPassword(newPassword, user->passwordHash);
    printf(Green("��������ɹ���"));
    pause();
}

void loginMenu() {
    while (true) {
        switch (displaySelect("�����רҵ�γ̹���ϵͳ", nullptr, OPTIONS_WITH_DEBUG(
            "��¼ϵͳ",
            "��������",
            "�˳�ϵͳ",
            "����"
        ))) {
        case 0:
            login();
            break;
        case 1:
            resetPassword();
            break;
        case 3:
            debugMenu();
            break;
        default:
            displayTitle("�����רҵ�γ̹���ϵͳ");
            printf("�ټ���\n");
            pause();
            return;
        }
    }
}

void saveAll(int error) {
    // �쳣�˳�֮ǰ�ȱ���һ��
    if (error != 0) {
        system("robocopy storage storage\\backup /E /XD backup >nul 2>&1");
    }

    CreateDirectory("storage", nullptr);
    CreateDirectory("storage\\data", nullptr);
    CreateDirectory("storage\\materials", nullptr);
    saveDataUser("storage\\data\\users.dat", UsersIndexId);
    saveDataCourse("storage\\data\\courses.dat", CoursesIndexId);
    saveDataTake("storage\\data\\takes.dat", TakesIndexCompound);
    saveDataMaterial("storage\\data\\materials.dat", MaterialsIndexId);
}

void signalHandler(int signal) {
    saveAll(signal);
    exit(signal);
}

BOOL WINAPI consoleHandler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        saveAll(-1);
        return TRUE;
    default:
        return FALSE;
    }
}

void parseCommandLine(int argc, char** argv) {
    bool startMain = false;

    int i = 0;
    while (i < argc) {
        if (strcmp(argv[i], "-addCourse") == 0) {
            char* filepath = argv[++i];
            FILE* fp = fopen(filepath, "r");
            if (!fp) {
                printf("���ļ� %s ʧ�ܡ�\n", filepath);
                exit(1);
            }
            Course* course = malloc(sizeof(Course));

            while (fscanf(fp, "%llu %d %llu %[^#]# %[^#]# %[^\n]\n", &course->id, &course->credit, &course->teacherId, course->name, course->time, course->classroom) == 6) {
                insertCourseRecord(course);
                printDebug("�Ѳ���γ� %s", course->name);
                course = malloc(sizeof(Course));
            }
            free(course);
        } else if (strcmp(argv[i], "-addUser") == 0) {
            char* filepath = argv[++i];
            FILE* fp = fopen(filepath, "r");
            if (!fp) {
                printf("���ļ� %s ʧ�ܡ�\n", filepath);
                exit(1);
            }
            User* user = malloc(sizeof(User));

            char passwordHash[crypto_pwhash_STRBYTES];
            setPassword("12345678", passwordHash);

            while (fscanf(fp, "%llu %s %s %u %u", &user->id, user->name, user->phoneNumber, &user->gender, &user->userType) == 5) {
                // ̫���� �ҵ�������Դ�����
                // setPassword("12345678", user->passwordHash);
                strcpy(user->passwordHash, passwordHash);

                if (user->userType == 1) {
                    fscanf(fp, "%s %[^\n]", user->academicTitle, user->office);
                } else if (user->userType == 2) {
                    fscanf(fp, "%d %s", &user->enrollmentYear, user->placeOfOrigin);
                }
                fscanf(fp, "\n");
                insertUserRecordById(user);
                printDebug("�Ѳ����û� %s", user->name);
                user = malloc(sizeof(User));
            }
            free(user);
        } else if (strcmp(argv[i], "-addTake") == 0) {
            char* filepath = argv[++i];
            FILE* fp = fopen(filepath, "r");
            if (!fp) {
                printf("���ļ� %s ʧ�ܡ�\n", filepath);
                exit(1);
            }
            Take* take = malloc(sizeof(Take));

            int takeCount = 0;
            while (fscanf(fp, "%llu %llu %d\n", &take->courseId, &take->studentId, &take->score) == 3) {
                insertTakeRecord(take);
                printDebug("�Ѳ���δ� #%d", takeCount + 1);
                take = malloc(sizeof(Take));
                takeCount++;
            }
            free(take);
        } else if (strcmp(argv[i], "-startMain")) {
            startMain = true;
        } else {
            printf("δʶ��������в�����%s\n", argv[i]);
            exit(1);
        }
        i++;
    }
    printf("\nAll done!\n");

    if (startMain) {
        pause();
        loginMenu();
    }
}

int main(int argc, char** argv) {
    system("title �����רҵ�γ̹���ϵͳ");
    printf("\033[?25l");
    sodium_init();

    // �û���ʹ�� id ����
    UsersIndexId = createTree(free, false);

    // �γ̣�ʹ�� id��teacherId �ֱ�����
    // ������������ʵ��ֱ�ӹ����¼���ڴ��ͷ�
    // ���Ǻ����������������߼�������������
    // ���Ի����������ֶ�����ɡ���ͬ
    CoursesIndexId = createTree(nullptr, false);
    CoursesIndexTeacherId = createTree(nullptr, true);

    // �δΣ�ʹ�� studentId & courseId��courseId �ֱ�����
    TakesIndexCompound = createTree(nullptr, false);
    TakesIndexCourseId = createTree(nullptr, true);

    // ���ϣ�ʹ�� id��courseId �ֱ�����
    MaterialsIndexId = createTree(nullptr, false);
    MaterialsIndexCourseId = createTree(nullptr, true);

    // ��ʼ������
    readDataUser("storage\\data\\users.dat", insertUserRecordById);
    readDataCourse("storage\\data\\courses.dat", insertCourseRecord);
    readDataTake("storage\\data\\takes.dat", insertTakeRecord);
    readDataMaterial("storage\\data\\materials.dat", insertMaterialRecord);

    if (DEBUG) {
        printDebug("����ȡ�� %d ���û�����", getTotalKeyCount(UsersIndexId));
        printDebug("����ȡ�� %d ���γ�����", getTotalKeyCount(CoursesIndexId));
        printDebug("����ȡ�� %d ���δ�����", getTotalKeyCount(TakesIndexCompound));
        printDebug("����ȡ�� %d ����������", getTotalKeyCount(MaterialsIndexId));
        pause();
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGABRT, signalHandler);
    SetConsoleCtrlHandler(consoleHandler, TRUE);

    if (argc > 1 && DEBUG) {
        parseCommandLine(argc - 1, argv + 1);
    } else {
        loginMenu();
    }

    // �������
    saveAll(0);
    printf("\033[?25h");

    return 0;
}