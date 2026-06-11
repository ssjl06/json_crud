#include <iostream>
#include <string>
#include <windows.h>
#include "member_store.h"

static const std::string DATA_FILE = "members.json";

// -------------------------------------------------------
// 출력 헬퍼
// -------------------------------------------------------
static void print_member(const Member& m)
{
    std::cout << "  ID         : " << m.id         << "\n";
    std::cout << "  이름       : " << m.name       << "\n";
    std::cout << "  이메일     : " << m.email      << "\n";
    std::cout << "  전화번호   : " << m.phone      << "\n";
    std::cout << "  생년월일   : " << m.birth_date << "\n";
    std::cout << "  주소       : " << m.address    << "\n";
}

// -------------------------------------------------------
// CRUD UI
// -------------------------------------------------------
static void create_member()
{
    std::cout << "\n=== 회원 등록 ===\n";
    auto members = load_from(DATA_FILE);

    Member m{};
    m.id = next_id(members);

    std::cout << "이름       : "; std::getline(std::cin, m.name);
    std::cout << "이메일     : "; std::getline(std::cin, m.email);
    std::cout << "전화번호   : "; std::getline(std::cin, m.phone);
    std::cout << "생년월일   : "; std::getline(std::cin, m.birth_date);
    std::cout << "주소       : "; std::getline(std::cin, m.address);

    members.push_back(m);
    save_to(DATA_FILE, members);
    std::cout << "\n[완료] ID " << m.id << " 회원이 등록되었습니다.\n";
}

static void read_all_members()
{
    std::cout << "\n=== 전체 회원 목록 ===\n";
    auto members = load_from(DATA_FILE);
    if (members.empty()) { std::cout << "등록된 회원이 없습니다.\n"; return; }
    for (const auto& m : members) {
        std::cout << "--------------------\n";
        print_member(m);
    }
}

static void read_member_by_id()
{
    std::cout << "\n=== 회원 조회 (ID) ===\n";
    std::cout << "조회할 ID : ";
    std::string input; std::getline(std::cin, input);

    int id;
    try { id = std::stoi(input); }
    catch (...) { std::cout << "잘못된 입력입니다.\n"; return; }

    auto members = load_from(DATA_FILE);
    auto found = find_by_id(members, id);
    if (found) {
        std::cout << "--------------------\n";
        print_member(*found);
    } else {
        std::cout << "ID " << id << " 회원을 찾을 수 없습니다.\n";
    }
}

static void do_update_member()
{
    std::cout << "\n=== 회원 수정 ===\n";
    std::cout << "수정할 ID : ";
    std::string input; std::getline(std::cin, input);

    int id;
    try { id = std::stoi(input); }
    catch (...) { std::cout << "잘못된 입력입니다.\n"; return; }

    auto members = load_from(DATA_FILE);
    auto found = find_by_id(members, id);
    if (!found) { std::cout << "ID " << id << " 회원을 찾을 수 없습니다.\n"; return; }

    std::cout << "\n현재 정보:\n";
    print_member(*found);
    std::cout << "\n새 정보를 입력하세요 (빈 칸 입력 시 기존 값 유지):\n";

    Member updated = *found;
    auto prompt = [](const std::string& label, std::string& field) {
        std::cout << label << " [" << field << "] : ";
        std::string buf; std::getline(std::cin, buf);
        if (!buf.empty()) field = buf;
    };

    prompt("이름      ", updated.name);
    prompt("이메일    ", updated.email);
    prompt("전화번호  ", updated.phone);
    prompt("생년월일  ", updated.birth_date);
    prompt("주소      ", updated.address);

    update_member(members, id, updated);
    save_to(DATA_FILE, members);
    std::cout << "\n[완료] ID " << id << " 회원 정보가 수정되었습니다.\n";
}

static void do_delete_member()
{
    std::cout << "\n=== 회원 삭제 ===\n";
    std::cout << "삭제할 ID : ";
    std::string input; std::getline(std::cin, input);

    int id;
    try { id = std::stoi(input); }
    catch (...) { std::cout << "잘못된 입력입니다.\n"; return; }

    auto members = load_from(DATA_FILE);
    if (remove_by_id(members, id)) {
        save_to(DATA_FILE, members);
        std::cout << "[완료] ID " << id << " 회원이 삭제되었습니다.\n";
    } else {
        std::cout << "ID " << id << " 회원을 찾을 수 없습니다.\n";
    }
}

// -------------------------------------------------------
// 메인 메뉴
// -------------------------------------------------------
int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::cout << "===========================\n";
    std::cout << "   회원 정보 관리 시스템\n";
    std::cout << "===========================\n";

    while (true) {
        std::cout << "\n[메뉴]\n";
        std::cout << "  1. 회원 등록\n";
        std::cout << "  2. 전체 목록 조회\n";
        std::cout << "  3. ID로 회원 조회\n";
        std::cout << "  4. 회원 정보 수정\n";
        std::cout << "  5. 회원 삭제\n";
        std::cout << "  0. 종료\n";
        std::cout << "선택 : ";

        std::string choice; std::getline(std::cin, choice);

        if      (choice == "1") create_member();
        else if (choice == "2") read_all_members();
        else if (choice == "3") read_member_by_id();
        else if (choice == "4") do_update_member();
        else if (choice == "5") do_delete_member();
        else if (choice == "0") { std::cout << "종료합니다.\n"; break; }
        else                    std::cout << "잘못된 선택입니다.\n";
    }

    return 0;
}
