#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <optional>
#include <windows.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// -------------------------------------------------------
// 데이터 파일 경로
// -------------------------------------------------------
static const std::string DATA_FILE = "members.json";

// -------------------------------------------------------
// 회원 구조체
// -------------------------------------------------------
struct Member {
    int         id;
    std::string name;       // 이름
    std::string email;      // 이메일
    std::string phone;      // 전화번호
    std::string birth_date; // 생년월일 (YYYY-MM-DD)
    std::string address;    // 주소
};

// -------------------------------------------------------
// JSON ↔ Member 변환
// -------------------------------------------------------
static Member from_json(const json& j)
{
    return {
        j.value("id",         0),
        j.value("name",       std::string{}),
        j.value("email",      std::string{}),
        j.value("phone",      std::string{}),
        j.value("birth_date", std::string{}),
        j.value("address",    std::string{}),
    };
}

static json to_json(const Member& m)
{
    return {
        {"id",         m.id},
        {"name",       m.name},
        {"email",      m.email},
        {"phone",      m.phone},
        {"birth_date", m.birth_date},
        {"address",    m.address},
    };
}

// -------------------------------------------------------
// 파일 I/O
// -------------------------------------------------------
static std::vector<Member> load_all()
{
    std::ifstream ifs(DATA_FILE);
    if (!ifs.is_open()) return {};

    json j;
    try { j = json::parse(ifs); }
    catch (const json::parse_error&) { return {}; }

    std::vector<Member> members;
    for (const auto& item : j)
        members.push_back(from_json(item));
    return members;
}

static void save_all(const std::vector<Member>& members)
{
    json arr = json::array();
    for (const auto& m : members)
        arr.push_back(to_json(m));

    std::ofstream ofs(DATA_FILE);
    ofs << arr.dump(4);
}

static int next_id(const std::vector<Member>& members)
{
    int max_id = 0;
    for (const auto& m : members)
        if (m.id > max_id) max_id = m.id;
    return max_id + 1;
}

// -------------------------------------------------------
// CRUD 함수
// -------------------------------------------------------
static void create_member()
{
    std::cout << "\n=== 회원 등록 ===\n";
    Member m;
    m.id = next_id(load_all());

    std::cout << "이름       : "; std::getline(std::cin, m.name);
    std::cout << "이메일     : "; std::getline(std::cin, m.email);
    std::cout << "전화번호   : "; std::getline(std::cin, m.phone);
    std::cout << "생년월일   : "; std::getline(std::cin, m.birth_date);
    std::cout << "주소       : "; std::getline(std::cin, m.address);

    auto members = load_all();
    members.push_back(m);
    save_all(members);

    std::cout << "\n[완료] ID " << m.id << " 회원이 등록되었습니다.\n";
}

static void print_member(const Member& m)
{
    std::cout << "  ID         : " << m.id         << "\n";
    std::cout << "  이름       : " << m.name       << "\n";
    std::cout << "  이메일     : " << m.email      << "\n";
    std::cout << "  전화번호   : " << m.phone      << "\n";
    std::cout << "  생년월일   : " << m.birth_date << "\n";
    std::cout << "  주소       : " << m.address    << "\n";
}

static void read_all_members()
{
    std::cout << "\n=== 전체 회원 목록 ===\n";
    auto members = load_all();
    if (members.empty()) {
        std::cout << "등록된 회원이 없습니다.\n";
        return;
    }
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

    auto members = load_all();
    for (const auto& m : members) {
        if (m.id == id) {
            std::cout << "--------------------\n";
            print_member(m);
            return;
        }
    }
    std::cout << "ID " << id << " 회원을 찾을 수 없습니다.\n";
}

static void update_member()
{
    std::cout << "\n=== 회원 수정 ===\n";
    std::cout << "수정할 ID : ";
    std::string input; std::getline(std::cin, input);

    int id;
    try { id = std::stoi(input); }
    catch (...) { std::cout << "잘못된 입력입니다.\n"; return; }

    auto members = load_all();
    for (auto& m : members) {
        if (m.id != id) continue;

        std::cout << "\n현재 정보:\n";
        print_member(m);
        std::cout << "\n새 정보를 입력하세요 (빈 칸 입력 시 기존 값 유지):\n";

        auto prompt = [](const std::string& label, std::string& field) {
            std::cout << label << " [" << field << "] : ";
            std::string buf; std::getline(std::cin, buf);
            if (!buf.empty()) field = buf;
        };

        prompt("이름      ", m.name);
        prompt("이메일    ", m.email);
        prompt("전화번호  ", m.phone);
        prompt("생년월일  ", m.birth_date);
        prompt("주소      ", m.address);

        save_all(members);
        std::cout << "\n[완료] ID " << id << " 회원 정보가 수정되었습니다.\n";
        return;
    }
    std::cout << "ID " << id << " 회원을 찾을 수 없습니다.\n";
}

static void delete_member()
{
    std::cout << "\n=== 회원 삭제 ===\n";
    std::cout << "삭제할 ID : ";
    std::string input; std::getline(std::cin, input);

    int id;
    try { id = std::stoi(input); }
    catch (...) { std::cout << "잘못된 입력입니다.\n"; return; }

    auto members = load_all();
    auto before = members.size();
    members.erase(
        std::remove_if(members.begin(), members.end(), [id](const Member& m) { return m.id == id; }),
        members.end()
    );

    if (members.size() == before) {
        std::cout << "ID " << id << " 회원을 찾을 수 없습니다.\n";
        return;
    }

    save_all(members);
    std::cout << "[완료] ID " << id << " 회원이 삭제되었습니다.\n";
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
        else if (choice == "4") update_member();
        else if (choice == "5") delete_member();
        else if (choice == "0") { std::cout << "종료합니다.\n"; break; }
        else                    std::cout << "잘못된 선택입니다.\n";
    }

    return 0;
}
