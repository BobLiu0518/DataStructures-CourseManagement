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

// 索引全局变量，命名按照 类型 + Index + 主键 的形式
// compound 表示复合主键，由所有主键计算而来
BPTree* UsersIndexId;
BPTree* CoursesIndexId, * CoursesIndexTeacherId;
BPTree* TakesIndexCompound, * TakesIndexCourseId;
BPTree* MaterialsIndexId, * MaterialsIndexCourseId;

// 少爷一声令下，管家就生成了 5 个内联函数，惊动全城
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
    displayTitle("管理系统 - 添加课程");
    Course* course = malloc(sizeof(Course));
    memset(course, 0, sizeof(Course));

    displayInput("课程编号", "%llu", &course->id);
    if (!course->id) {
        printf(Red("失败：")"必须输入课程编号。");
        free(course);
        pause();
        return;
    } else if (findRecord(CoursesIndexId, course->id)) {
        printf(Red("失败：")"课程已存在。");
        free(course);
        pause();
        return;
    }
    displayInput("教师工号", "%llu", &course->teacherId);
    User* teacher = findRecord(UsersIndexId, course->teacherId);
    if (!teacher || teacher->userType != USER_TEACHER) {
        printf(Red("失败：")"教师不存在。");
        free(course);
        pause();
        return;
    }
    displayInput("课程名称", "%[^\n]", course->name);
    double credit;
    displayInput("课程学分", "%lf", &credit);
    course->credit = credit * 10;
    displayInput("上课时间", "%[^\n]", course->time);
    displayInput("上课教室", "%[^\n]", course->classroom);

    insertRecord(CoursesIndexId, course->id, course);
    insertRecord(CoursesIndexTeacherId, course->teacherId, course);
    printf(Green("添加课程 %s 成功。"), course->name);
    pause();
}

Course* selectCourse(RecordArray courses) {
    int currentPage = 0, totalPages;
    char pageInfo[41], * options[9];
    Course* selection = nullptr;

    if (!courses.total) {
        displayTitle("管理系统 - 选择课程");
        printf("无可选课程");
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
        sprintf(pageInfo, "第 %d 页 / 共 %d 页", currentPage + 1, totalPages);
        if (!isFirstPage) {
            strcpy(options[count++], "(上一页)");
        }
        while (count < (isLastPage ? 9 : 8) && currentPage * 7 + !isFirstPage + count < courses.total) {
            strcpy(options[count], ((Course*)courses.arr[currentPage * 7 + !isFirstPage + count])->name);
            count++;
        }
        if (!isLastPage) {
            strcpy(options[count++], "(下一页)");
        }
        int choice = displaySelect("管理系统 - 选择课程", pageInfo, count, options);
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
        printf("课程编号: "BlackBright("%llu\n"), course->id);
    }
    printf("课程名称: "BlackBright("%s\n"), course->name);
    printf("课程学分: "BlackBright("%.1lf\n"), course->credit / 10.0);
    printf("上课时间: "BlackBright("%s\n"), course->time);
    printf("上课教室: "BlackBright("%s\n"), course->classroom);

    User* teacher = findRecord(UsersIndexId, course->teacherId);
    if (!teacher) {
        printf("任课教师: "BlackBright("未知\n"));
    } else {
        printf("任课教师: "BlackBright("%s[%s] (%llu)\n"), teacher->name, teacher->academicTitle, teacher->id);
    }
    pause();
}

void listCourses() {
    Course* course = selectCourse(findRecordRangeArray(CoursesIndexId, 0, UINT64_MAX));
    if (course) {
        displayTitle("管理系统 - 课程信息");
        showCourseDetail(course, true);
    }
}

void queryCourse() {
    displayTitle("管理系统 - 查询课程");
    Key courseId;

    displayInput("课程编号", "%llu", &courseId);
    Course* course = findRecord(CoursesIndexId, courseId);
    if (!course) {
        printf(Yellow("未找到课程 %llu。"), courseId);
        pause();
        return;
    }

    showCourseDetail(course, false);
}


void statScore(Course* course, bool showTeacher) {
    displayTitle("管理系统 - 成绩统计");
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

    printf("课程: "BlackBright("%s\n"), course->name);
    if (showTeacher) {
        User* teacher = findRecord(UsersIndexId, course->teacherId);
        printf("教师: "BlackBright("%s [%s] (%llu)\n"), teacher->name, teacher->academicTitle, teacher->id);
    }
    printf("共有学生 %2d 名，其中成绩未录入 %2d 名\n", takes.total, takes.total - setCount);
    if (setCount) {
        printf("已录入成绩的学生中：\n");
        printf("　优秀 %2d 名 (%5.2lf%%)\n", excellentCount, excellentCount * 100.0 / setCount);
        printf("　合格 %2d 名 (%5.2lf%%)\n", passedCount, passedCount * 100.0 / setCount);
        printf("不合格 %2d 名 (%5.2lf%%)\n", failedCount, failedCount * 100.0 / setCount);
        printf("班级平均分 %5.2lf 分\n", totalScore * 0.1 / setCount);
        printf("最高分 %4.1lf 分：%s\n", highestScore / 10.0, highestOwner);
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
    displayTitle("管理系统 - 修改课程");
    Key courseId;

    displayInput("课程编号", "%llu", &courseId);
    Course* course = findRecord(CoursesIndexId, courseId);
    if (!course) {
        printf(Red("失败：")"课程不存在。");
        pause();
        return;
    }

    while (true) {
        char courseInfo[100];
        sprintf(courseInfo, "%s (%llu)", course->name, course->id);
        int choice = displaySelect("管理系统 - 修改课程", courseInfo, OPTIONS("修改名称", "修改学分", "修改教师", "退出修改"));
        displayTitle("管理系统 - 修改课程");
        switch (choice) {
        case 0:
            displayInput("课程名称", "%[^\n]", course->name);
            break;
        case 1:
            double credit;
            displayInput("课程学分", "%lf", &credit);
            course->credit = credit * 10;
            break;
        case 2:
            Key teacherId;
            displayInput("教师工号", "%llu", &teacherId);
            User* teacher = findRecord(UsersIndexId, teacherId);
            if (!teacher || teacher->userType != USER_TEACHER) {
                printf(Red("失败：")"教师不存在。");
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
        printf(Green("修改成功。"));
        pause();
    }
}

void deleteCourse() {
    displayTitle("管理系统 - 删除课程");
    Key courseId;

    displayInput("课程编号", "%llu", &courseId);
    Course* course = findRecord(CoursesIndexId, courseId);
    if (!course) {
        printf(Red("失败：")"课程不存在。");
        pause();
        return;
    }
    char prompt[100];
    sprintf(prompt, "确定要删除课程 %s 吗？", course->name);
    if (displaySelect("管理系统 - 删除课程", prompt, OPTIONS("否", "是")) != 1) {
        return;
    }

    displayTitle("管理系统 - 删除课程");
    removeRecord(CoursesIndexId, courseId, nullptr);
    removeRecord(CoursesIndexTeacherId, course->teacherId, course);
    free(course);
    printf(Green("删除课程成功。"));
    pause();

    // 本来是想这么做的，但是突然想到一边遍历一边删好像会出问题，等有时间再优化吧
    // Edit: 现在可以用 findRecordRangeArray 实现了，先缓存再删
    // TODO
    // if (displaySelect("管理系统 - 删除课程", "是否要删除该课程的相关数据？", OPTIONS("是", "否")) != 0) {
    //     return;
    // }
    // 
    // findRecordRange(TakesIndexCourseId, courseId, courseId, removeTake); // 不对！
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
    displayTitle("管理系统 - 查询平均绩点");

    if (!student) {
        Key studentId;
        displayInput("学生学号", "%llu", &studentId);
        student = findRecord(UsersIndexId, studentId);
        if (!student) {
            printf(Red("错误：")"学生不存在。");
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
    printf("学生: "BlackBright("%s\n"), student->name);
    printf("已修得总学分: "BlackBright("%.1lf\n"), totalCredit / 10.0);
    if (takes.total) {
        printf("平均学分绩点: "BlackBright("%.2lf\n"), totalCreditGradePoint / 10.0 / totalCredit);
    }

    free(takes.arr);
    pause();
}

Material* selectMaterial(RecordArray materials) {
    char** options = calloc(materials.total, sizeof(char*));

    if (!materials.total) {
        displayTitle("管理系统 - 选择学习资料");
        printf("无可选资料");
        pause();
        return nullptr;
    }

    for (int i = 0; i < materials.total; i++) {
        options[i] = ((Material*)materials.arr[i])->title;
    }

    int selection = displaySelect("管理系统 - 选择学习资料", nullptr, materials.total, options);
    Material* material = selection != -1 ? materials.arr[selection] : nullptr;

    free(options);
    free(materials.arr);
    return material;
}

void listAllMaterials() {
    Material* material = selectMaterial(findRecordRangeArray(MaterialsIndexId, 0, UINT64_MAX));
    if (material) {
        displayTitle("管理系统 - 查看学习资料");
        printf("正在查看学习资料…\n");
        system(material->path);
    }
}

void statAvgSelected() {
    displayTitle("管理系统 - 统计平均选课");
    Key teacherId;
    displayInput("教师工号", "%llu", &teacherId);
    User* teacher = findRecord(UsersIndexId, teacherId);
    if (!teacher) {
        printf(Red("错误：")"教师不存在。");
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

    printf("教师: "BlackBright("%s [%s] (%llu)\n"), teacher->name, teacher->academicTitle, teacher->id);
    printf("总共开设 %d 门课程\n", courses.total);
    if (courses.total) {
        printf("平均选课人数为 %.1lf 人\n", totalStudents * 1.0 / courses.total);
    }
    pause();
}

void adminMenu(User* user) {
    char welcome[41];
    sprintf(welcome, "欢迎，%s！", user->name);
    while (true) {
        switch (displaySelect("管理系统 - 管理员", welcome, OPTIONS(
            "添加课程",
            "浏览课程",
            "查询课程",
            "修改课程",
            "删除课程",
            "查询学生平均绩点",
            "查看全部学习资料",
            "统计课程成绩情况",
            "统计教师平均选课人数",
            "退出登录"
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
    displayTitle("管理系统 - 添加学习资料");
    printf("请将学习资料放置在\nstorage\\materials 文件夹下。\n");
    displayInput("文件名", "%[^\n]", filepath + strlen(filepath));

    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        printf(Red("失败：")"文件不存在。\n");
        pause();
        return;
    }
    fclose(fp);
    displayInput("标题", "%[^\n]", title);

    Material* material = malloc(sizeof(Material));
    material->id = getBiggestKey(MaterialsIndexId) + 1;
    material->courseId = course->id;
    strcpy(material->title, title);
    strcpy(material->path, filepath);

    insertMaterialRecord(material);
    printf(Green("添加学习资料成功。"));
    pause();
}

void showMaterial(Course* course) {
    Material* material = selectMaterial(findRecordRangeArray(MaterialsIndexCourseId, course->id, course->id));
    if (material) {
        displayTitle("管理系统 - 查看学习资料");
        printf("正在查看学习资料…\n");
        system(material->path);
    }
}

void modifyMaterial(Course* course) {
    Material* material = selectMaterial(findRecordRangeArray(MaterialsIndexCourseId, course->id, course->id));
    if (!material) {
        return;
    }

    while (true) {
        int choice = displaySelect("管理系统 - 修改学习资料", material->title, OPTIONS(
            "修改资料标题",
            "修改资料文件",
            "退出修改"
        ));
        displayTitle("管理系统 - 修改学习资料");
        switch (choice) {
        case 0:
            displayInput("标题", "%[^\n]", material->title);
            break;
        case 1:
            char filepath[DATA_MATERIAL_PATH_LENGTH + 1] = "storage\\materials\\";
            printf("请将学习资料放置在\nstorage\\materials 文件夹下。\n");
            displayInput("文件名", "%[^\n]", filepath + strlen(filepath));

            FILE* fp = fopen(filepath, "r");
            if (!fp) {
                printf(Red("错误：")"文件%s不存在。", filepath);
                pause();
                continue;
            }
            fclose(fp);
            break;
        default:
            return;
        }
        printf(Green("修改成功。"));
        pause();
    }
}

void deleteMaterial(Course* course) {
    Material* material = selectMaterial(findRecordRangeArray(MaterialsIndexCourseId, course->id, course->id));
    if (!material) {
        return;
    }

    char prompt[100];
    sprintf(prompt, "确定要删除资料 %s 吗？", material->title);
    if (displaySelect("管理系统 - 删除学习资料", prompt, OPTIONS("否", "是")) != 1) {
        return;
    }

    displayTitle("管理系统 - 删除学习资料");
    DeleteFile(material->path);
    removeRecord(MaterialsIndexId, material->id, nullptr);
    removeRecord(MaterialsIndexCourseId, material->courseId, material);
    free(material);
    printf(Green("删除学习资料成功。"));
    pause();
}

void modifyScore(Course* course) {
    displayTitle("管理系统 - 录入成绩");
    Key studentId;
    displayInput("学生学号", "%llu", &studentId);
    User* student = findRecord(UsersIndexId, studentId);
    if (!student || student->userType != USER_STUDENT) {
        printf(Red("错误：")"未找到学生。");
        pause();
        return;
    }

    Take* take = findRecord(TakesIndexCompound, student->id * 100000000ULL + course->id);
    if (!take) {
        printf(Yellow("学生 %s 未选课程 %s。"), student->name, course->name);
        pause();
        return;
    }

    printf("学生: "BlackBright("%s (%llu)\n"), student->name, student->id);
    printf("课程: "BlackBright("%s (%llu)\n"), course->name, course->id);
    if (take->score != DATA_SCORE_NOT_SET) {
        printf("成绩: "BlackBright("%.1lf\n"), take->score / 10.0);
        pause();
        if (displaySelect("管理系统 - 录入成绩", "成绩已存在，是否重新录入？", OPTIONS("否", "是")) != 1) {
            return;
        }
        displayTitle("管理系统 - 录入成绩");
        printf("学生: "BlackBright("%s (%llu)\n"), student->name, student->id);
        printf("课程: "BlackBright("%s (%llu)\n"), course->name, course->id);
    }

    double score;
    displayInput("成绩", "%lf", &score);
    if (score < 0 && score > 100) {
        printf(Red("错误：")"成绩不合法。");
        pause();
        return;
    }
    take->score = score * 10;
    printf(Green("录入成绩成功。"));
    pause();
}

void studentList(Course* course) {
    displayTitle("管理系统 - 学生名单");
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
        switch (displaySelect("管理系统 - 课程管理", course->name, OPTIONS(
            "课程信息",
            "添加学习资料",
            "查看学习资料",
            "修改学习资料",
            "删除学习资料",
            "学生名单",
            "统计成绩",
            "录入成绩",
            "退出管理"
        ))) {
        case 0:
            displayTitle("管理系统 - 课程信息");
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
    sprintf(welcome, "欢迎，%s！", user->name);
    while (true) {
        switch (displaySelect("管理系统 - 教师", welcome, OPTIONS(
            "课程管理",
            "退出登录"
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
        displayTitle("管理系统 - 课程信息");
        printf("你还未选过这门课。\n");
        pause();
        return;
    }
    while (true) {
        switch (displaySelect("管理系统 - 课程信息", course->name, OPTIONS(
            "课程信息查询",
            "课程成绩查询",
            "学习资料查询",
            "退出课程信息"
        ))) {
        case 0:
            displayTitle("管理系统 - 课程信息");
            showCourseDetail(course, true);
            break;
        case 1:
            displayTitle("管理系统 - 课程成绩");
            printf("学生: "BlackBright("%s (%llu)\n"), user->name, user->id);
            printf("课程: "BlackBright("%s (%llu)\n"), course->name, course->id);
            if (take->score == DATA_SCORE_NOT_SET) {
                printf(Yellow("成绩尚未录入，请联系任课教师\n"));
            } else if (take->score < 600) {
                printf(Yellow("总评成绩 %.1lf，不合格\n"), take->score / 10.0);
            } else if (take->score < 900) {
                printf(Green("总评成绩 %.1lf，合格\n"), take->score / 10.0);
            } else {
                printf(Green("总评成绩 %.1lf，优秀\n"), take->score / 10.0);
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
    sprintf(welcome, "欢迎，%s！", user->name);
    while (true) {
        switch (displaySelect("管理系统 - 学生", welcome, OPTIONS(
            "课程信息",
            "查询平均绩点",
            "退出登录"
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
    displayTitle("调试 - B+ 树测试");
    displayInput("循环测试次数", "%d", &count);
    displayInput("测试插入次数", "%d", &addCount);
    displayInput("测试删除次数", "%d", &removeCount);
    displayInput("是否允许重复", "%d", &allowDuplicateKey);
    if (count < 0 || removeCount > addCount || addCount > 100) {
        printf(Red("失败：")"参数错误。");
        pause();
        return;
    }
    displayInput("随机种子", "%u", &seed);
    if (!seed) {
        seed = time(NULL);
    }
    srand(seed);
    printDebug("使用的随机种子：%u", seed);

    CreateDirectory("storage\\debug", nullptr);
    for (int t = 0; !count || t < count; t++) {
        Key data[addCount];
        char filename[50];
        BPTree* tree = createTree(nullptr, allowDuplicateKey != 0);
        for (int i = 0; i < addCount; i++) {
            data[i] = rand() % 100;
            printDebug("[%03d] 尝试插入 %d", i + 1, data[i]);
            if (insertRecord(tree, data[i], data + i)) {
                sprintf(filename, "storage\\debug\\test%03d.html", i + 1);
                saveTreeMermaid(tree, filename, false);
                checkTreeLegitimacy(tree, true);
            } else {
                printDebug("插入失败：重复。");
                i--;
            }
        }
        for (int i = addCount; i < addCount + removeCount; i++) {
            int removeIndex = rand() % addCount;
            int removeData = data[removeIndex];
            printDebug("[%03d] 尝试删除 %d", i + 1, removeData);
            if (removeRecord(tree, removeData, data + removeIndex)) {
                sprintf(filename, "storage\\debug\\test%03d.html", i + 1);
                saveTreeMermaid(tree, filename, false);
                checkTreeLegitimacy(tree, true);
            } else {
                printDebug("删除失败：未找到。");
                i--;
            }
        }
        printDebug("正在销毁测试树…");
        destroyTree(tree);
    }
    printf(Green("\n好耶！全部完成！"));
    pause();
}

// 调试用，没有做过多的参数验证
void addUser() {
    displayTitle("调试 - 添加用户");
    User* user = malloc(sizeof(User));
    char password[DATA_PASSWORD_LENGTH + 1];

    displayInput("学工号", "%llu", &user->id);
    displayInputPassword("初始密码", password, DATA_PASSWORD_LENGTH);
    setPassword(password, user->passwordHash);
    displayInput("姓名", "%s", user->name);
    displayInput("手机号", "%s", user->phoneNumber);
    user->gender = displaySelect("调试 - 添加用户", "选择性别", OPTIONS("女", "男"));
    user->userType = displaySelect("调试 - 添加用户", "选择用户类型", OPTIONS("管理员", "教师", "学生"));

    displayTitle("调试 - 添加用户");
    switch (user->userType) {
    case USER_ADMIN:
        break;
    case USER_TEACHER:
        displayInput("职称", "%s", user->academicTitle);
        displayInput("办公室", "%[^\n]", user->office);
        break;
    case USER_STUDENT:
        displayInput("年级", "%d", &user->enrollmentYear);
        displayInput("生源地", "%s", user->placeOfOrigin);
        break;
    }

    insertRecord(UsersIndexId, user->id, user);
    printf(Green("添加成功。"));
    pause();
}

void deleteUser() {
    displayTitle("调试 - 删除用户");

    Key id;
    displayInput("学工号", "%llu", &id);
    if (removeRecord(UsersIndexId, id, nullptr)) {
        printf(Green("删除成功。"));
    } else {
        printf(Red("错误：")"未找到该用户。");
    }
    pause();
}

void debugMenu() {
    while (true) {
        switch (displaySelect("管理系统 - 调试", nullptr, OPTIONS(
            "测试 B+ 树",
            "添加用户",
            "删除用户",
            "退出"
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
    displayTitle("管理系统 - 登录");
    displayInput("学工号", "%llu", &username);
    User* user = findRecord(UsersIndexId, username);
    if (!user) {
        printf(Red("登录失败：")"用户不存在。");
        pause();
        return;
    }
    displayInputPassword("密码", password, DATA_PASSWORD_LENGTH);
    if (verifyPassword(password, user->passwordHash) != 0) {
        printf(Red("登录失败：")"密码错误。");
        pause();
        return;
    }
    printf(Green("你好，%s！"), user->name);
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
    displayTitle("管理系统 - 忘记密码");
    displayInput("学工号", "%llu", &username);
    User* user = findRecord(UsersIndexId, username);
    if (!user) {
        printf(Red("失败：")"用户不存在。");
        pause();
        return;
    }
    strcpy(phoneNumber, user->phoneNumber);
    for (int i = 3; i < 7; i++) {
        phoneNumber[i] = '*';
    }
    printf("验证码已发送至 %s。\n", phoneNumber); // 并没有
    displayInput("验证码", "%s", verifyCode);
    if (strlen(verifyCode) != 6) {
        printf(Red("失败：")"验证码错误。");
        pause();
        return;
    }
    displayInputPassword("输入新密码", newPassword, DATA_PASSWORD_LENGTH);
    if (strlen(newPassword) < 8) {
        printf(Red("失败：")"密码至少为 8 位。");
        pause();
        return;
    }
    setPassword(newPassword, user->passwordHash);
    printf(Green("重设密码成功。"));
    pause();
}

void loginMenu() {
    while (true) {
        switch (displaySelect("计算机专业课程管理系统", nullptr, OPTIONS_WITH_DEBUG(
            "登录系统",
            "忘记密码",
            "退出系统",
            "调试"
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
            displayTitle("计算机专业课程管理系统");
            printf("再见！\n");
            pause();
            return;
        }
    }
}

void saveAll(int error) {
    // 异常退出之前先备份一下
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
                printf("打开文件 %s 失败。\n", filepath);
                exit(1);
            }
            Course* course = malloc(sizeof(Course));

            while (fscanf(fp, "%llu %d %llu %[^#]# %[^#]# %[^\n]\n", &course->id, &course->credit, &course->teacherId, course->name, course->time, course->classroom) == 6) {
                insertCourseRecord(course);
                printDebug("已插入课程 %s", course->name);
                course = malloc(sizeof(Course));
            }
            free(course);
        } else if (strcmp(argv[i], "-addUser") == 0) {
            char* filepath = argv[++i];
            FILE* fp = fopen(filepath, "r");
            if (!fp) {
                printf("打开文件 %s 失败。\n", filepath);
                exit(1);
            }
            User* user = malloc(sizeof(User));

            char passwordHash[crypto_pwhash_STRBYTES];
            setPassword("12345678", passwordHash);

            while (fscanf(fp, "%llu %s %s %u %u", &user->id, user->name, user->phoneNumber, &user->gender, &user->userType) == 5) {
                // 太慢了 我的老年电脑带不动
                // setPassword("12345678", user->passwordHash);
                strcpy(user->passwordHash, passwordHash);

                if (user->userType == 1) {
                    fscanf(fp, "%s %[^\n]", user->academicTitle, user->office);
                } else if (user->userType == 2) {
                    fscanf(fp, "%d %s", &user->enrollmentYear, user->placeOfOrigin);
                }
                fscanf(fp, "\n");
                insertUserRecordById(user);
                printDebug("已插入用户 %s", user->name);
                user = malloc(sizeof(User));
            }
            free(user);
        } else if (strcmp(argv[i], "-addTake") == 0) {
            char* filepath = argv[++i];
            FILE* fp = fopen(filepath, "r");
            if (!fp) {
                printf("打开文件 %s 失败。\n", filepath);
                exit(1);
            }
            Take* take = malloc(sizeof(Take));

            int takeCount = 0;
            while (fscanf(fp, "%llu %llu %d\n", &take->courseId, &take->studentId, &take->score) == 3) {
                insertTakeRecord(take);
                printDebug("已插入课次 #%d", takeCount + 1);
                take = malloc(sizeof(Take));
                takeCount++;
            }
            free(take);
        } else if (strcmp(argv[i], "-startMain")) {
            startMain = true;
        } else {
            printf("未识别的命令行参数：%s\n", argv[i]);
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
    system("title 计算机专业课程管理系统");
    printf("\033[?25l");
    sodium_init();

    // 用户：使用 id 索引
    UsersIndexId = createTree(free, false);

    // 课程：使用 id、teacherId 分别索引
    // 本来是想用树实现直接管理记录的内存释放
    // 但是后来发现这样代码逻辑反而更复杂了
    // 所以还是在这里手动管理吧。下同
    CoursesIndexId = createTree(nullptr, false);
    CoursesIndexTeacherId = createTree(nullptr, true);

    // 课次：使用 studentId & courseId、courseId 分别索引
    TakesIndexCompound = createTree(nullptr, false);
    TakesIndexCourseId = createTree(nullptr, true);

    // 材料：使用 id、courseId 分别索引
    MaterialsIndexId = createTree(nullptr, false);
    MaterialsIndexCourseId = createTree(nullptr, true);

    // 初始化数据
    readDataUser("storage\\data\\users.dat", insertUserRecordById);
    readDataCourse("storage\\data\\courses.dat", insertCourseRecord);
    readDataTake("storage\\data\\takes.dat", insertTakeRecord);
    readDataMaterial("storage\\data\\materials.dat", insertMaterialRecord);

    if (DEBUG) {
        printDebug("共读取到 %d 条用户数据", getTotalKeyCount(UsersIndexId));
        printDebug("共读取到 %d 条课程数据", getTotalKeyCount(CoursesIndexId));
        printDebug("共读取到 %d 条课次数据", getTotalKeyCount(TakesIndexCompound));
        printDebug("共读取到 %d 条资料数据", getTotalKeyCount(MaterialsIndexId));
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

    // 保存更改
    saveAll(0);
    printf("\033[?25h");

    return 0;
}