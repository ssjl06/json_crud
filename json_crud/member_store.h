#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// -------------------------------------------------------
// 회원 구조체
// -------------------------------------------------------
struct Member {
    int         id;
    std::string name;
    std::string email;
    std::string phone;
    std::string birth_date;
    std::string address;
};

// -------------------------------------------------------
// JSON ↔ Member 변환
// -------------------------------------------------------
inline Member member_from_json(const json& j)
{
    auto str = [&](const char* key) -> std::string {
        auto it = j.find(key);
        if (it != j.end() && it->is_string())
            return it->get<std::string>();
        return {};
    };

    int id = 0;
    auto it = j.find("id");
    if (it != j.end() && it->is_number_integer())
        id = it->get<int>();

    return { id, str("name"), str("email"), str("phone"), str("birth_date"), str("address") };
}

inline json member_to_json(const Member& m)
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
// 파일 I/O (경로를 인자로 받아 테스트 가능)
// -------------------------------------------------------
inline std::vector<Member> load_from(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) return {};

    json j;
    try { j = json::parse(ifs); }
    catch (const json::parse_error&) { return {}; }

    if (!j.is_array()) return {};

    std::vector<Member> members;
    for (const auto& item : j) {
        try { members.push_back(member_from_json(item)); }
        catch (...) { /* 손상된 레코드 건너뜀 */ }
    }
    return members;
}

inline void save_to(const std::string& path, const std::vector<Member>& members)
{
    json arr = json::array();
    for (const auto& m : members)
        arr.push_back(member_to_json(m));

    std::ofstream ofs(path);
    ofs << arr.dump(4);
}

// -------------------------------------------------------
// 순수 로직 (파일 I/O 없음, 테스트 용이)
// -------------------------------------------------------
inline int next_id(const std::vector<Member>& members)
{
    int max_id = 0;
    for (const auto& m : members)
        if (m.id > max_id) max_id = m.id;
    return max_id + 1;
}

inline std::optional<Member> find_by_id(const std::vector<Member>& members, int id)
{
    for (const auto& m : members)
        if (m.id == id) return m;
    return std::nullopt;
}

// 수정 성공 시 true 반환, id 제외 모든 필드 교체
inline bool update_member(std::vector<Member>& members, int id, const Member& updated)
{
    for (auto& m : members) {
        if (m.id != id) continue;
        m.name       = updated.name;
        m.email      = updated.email;
        m.phone      = updated.phone;
        m.birth_date = updated.birth_date;
        m.address    = updated.address;
        return true;
    }
    return false;
}

// 삭제 성공 시 true 반환
inline bool remove_by_id(std::vector<Member>& members, int id)
{
    auto before = members.size();
    members.erase(
        std::remove_if(members.begin(), members.end(),
            [id](const Member& m) { return m.id == id; }),
        members.end()
    );
    return members.size() != before;
}
