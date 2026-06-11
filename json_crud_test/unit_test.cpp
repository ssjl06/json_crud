#include <iostream>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <climits>
#include <windows.h>
#include "member_store.h"

namespace fs = std::filesystem;

// -------------------------------------------------------
// 간단한 테스트 프레임워크
// -------------------------------------------------------
static int s_passed = 0, s_failed = 0;

#define ASSERT_EQ(a, b) do { \
    if (!((a) == (b))) { \
        throw std::runtime_error( \
            std::string("ASSERT_EQ 실패 [line ") + std::to_string(__LINE__) + \
            "]: (" + #a + ") != (" + #b + ")"); \
    } \
} while(0)

#define ASSERT_TRUE(x)  ASSERT_EQ((x), true)
#define ASSERT_FALSE(x) ASSERT_EQ((x), false)
#define ASSERT_STREQ(a, b) do { \
    if (std::string(a) != std::string(b)) { \
        throw std::runtime_error( \
            std::string("ASSERT_STREQ 실패 [line ") + std::to_string(__LINE__) + \
            "]: \"" + (a) + "\" != \"" + (b) + "\""); \
    } \
} while(0)

static void run_test(const std::string& name, std::function<void()> fn)
{
    try {
        fn();
        std::cout << "[PASS] " << name << "\n";
        s_passed++;
    } catch (const std::exception& e) {
        std::cout << "[FAIL] " << name << " : " << e.what() << "\n";
        s_failed++;
    }
}

// 테스트용 임시 파일 경로
static std::string tmp(const std::string& id)
{
    return (fs::temp_directory_path() / ("member_test_" + id + ".json")).string();
}

static void del(const std::string& path)
{
    fs::remove(path);
}

// -------------------------------------------------------
// 테스트용 회원 데이터 팩토리
// -------------------------------------------------------
static Member make_member(int id, const std::string& name = "홍길동",
    const std::string& email   = "hong@test.com",
    const std::string& phone   = "010-1234-5678",
    const std::string& birth   = "1990-01-01",
    const std::string& address = "서울시 강남구")
{
    return { id, name, email, phone, birth, address };
}

// ===============================================================
// [UNIT TESTS] Regression
// ===============================================================

static void test_member_to_json()
{
    Member m = make_member(1);
    json j = member_to_json(m);
    ASSERT_EQ(j["id"].get<int>(), 1);
    ASSERT_STREQ(j["name"].get<std::string>(), "홍길동");
    ASSERT_STREQ(j["email"].get<std::string>(), "hong@test.com");
    ASSERT_STREQ(j["phone"].get<std::string>(), "010-1234-5678");
    ASSERT_STREQ(j["birth_date"].get<std::string>(), "1990-01-01");
    ASSERT_STREQ(j["address"].get<std::string>(), "서울시 강남구");
}

static void test_member_from_json()
{
    json j = {
        {"id", 42}, {"name", "김철수"}, {"email", "kim@test.com"},
        {"phone", "010-9999-0000"}, {"birth_date", "1985-06-15"}, {"address", "부산시 해운대구"}
    };
    Member m = member_from_json(j);
    ASSERT_EQ(m.id, 42);
    ASSERT_STREQ(m.name, "김철수");
    ASSERT_STREQ(m.email, "kim@test.com");
}

static void test_json_roundtrip()
{
    Member original = make_member(7, "이영희", "lee@test.com", "010-5555-6666", "2000-12-31", "인천시 남동구");
    Member restored = member_from_json(member_to_json(original));
    ASSERT_EQ(restored.id, original.id);
    ASSERT_STREQ(restored.name,       original.name);
    ASSERT_STREQ(restored.email,      original.email);
    ASSERT_STREQ(restored.phone,      original.phone);
    ASSERT_STREQ(restored.birth_date, original.birth_date);
    ASSERT_STREQ(restored.address,    original.address);
}

static void test_next_id_empty()
{
    ASSERT_EQ(next_id({}), 1);
}

static void test_next_id_sequential()
{
    std::vector<Member> members = { make_member(1), make_member(2), make_member(3) };
    ASSERT_EQ(next_id(members), 4);
}

static void test_next_id_with_gaps()
{
    // 연속되지 않아도 최대값 + 1
    std::vector<Member> members = { make_member(1), make_member(5), make_member(3) };
    ASSERT_EQ(next_id(members), 6);
}

static void test_find_by_id_found()
{
    std::vector<Member> members = { make_member(1, "A"), make_member(2, "B"), make_member(3, "C") };
    auto result = find_by_id(members, 2);
    ASSERT_TRUE(result.has_value());
    ASSERT_STREQ(result->name, "B");
}

static void test_find_by_id_not_found()
{
    std::vector<Member> members = { make_member(1), make_member(2) };
    ASSERT_FALSE(find_by_id(members, 99).has_value());
}

static void test_find_by_id_empty_list()
{
    ASSERT_FALSE(find_by_id({}, 1).has_value());
}

static void test_update_member_success()
{
    std::vector<Member> members = { make_member(1, "원래이름") };
    Member updated = make_member(1, "변경이름");
    bool ok = update_member(members, 1, updated);
    ASSERT_TRUE(ok);
    ASSERT_STREQ(members[0].name, "변경이름");
}

static void test_update_member_not_found()
{
    std::vector<Member> members = { make_member(1, "원래이름") };
    bool ok = update_member(members, 99, make_member(99, "변경이름"));
    ASSERT_FALSE(ok);
    ASSERT_STREQ(members[0].name, "원래이름"); // 변경 없음
}

static void test_update_preserves_id()
{
    std::vector<Member> members = { make_member(5, "이름") };
    Member u = make_member(5, "새이름");
    update_member(members, 5, u);
    ASSERT_EQ(members[0].id, 5); // id 불변
}

static void test_update_all_fields()
{
    std::vector<Member> members = { make_member(1, "A", "a@a.com", "000", "1111-11-11", "주소A") };
    Member u = { 1, "B", "b@b.com", "999", "2222-22-22", "주소B" };
    update_member(members, 1, u);
    ASSERT_STREQ(members[0].name,       "B");
    ASSERT_STREQ(members[0].email,      "b@b.com");
    ASSERT_STREQ(members[0].phone,      "999");
    ASSERT_STREQ(members[0].birth_date, "2222-22-22");
    ASSERT_STREQ(members[0].address,    "주소B");
}

static void test_remove_by_id_success()
{
    std::vector<Member> members = { make_member(1), make_member(2), make_member(3) };
    bool ok = remove_by_id(members, 2);
    ASSERT_TRUE(ok);
    ASSERT_EQ(static_cast<int>(members.size()), 2);
    ASSERT_FALSE(find_by_id(members, 2).has_value());
}

static void test_remove_by_id_not_found()
{
    std::vector<Member> members = { make_member(1), make_member(2) };
    bool ok = remove_by_id(members, 99);
    ASSERT_FALSE(ok);
    ASSERT_EQ(static_cast<int>(members.size()), 2);
}

static void test_remove_preserves_others()
{
    std::vector<Member> members = { make_member(1,"A"), make_member(2,"B"), make_member(3,"C") };
    remove_by_id(members, 2);
    ASSERT_STREQ(members[0].name, "A");
    ASSERT_STREQ(members[1].name, "C");
}

static void test_remove_all_members()
{
    std::vector<Member> members = { make_member(1), make_member(2), make_member(3) };
    remove_by_id(members, 1);
    remove_by_id(members, 2);
    remove_by_id(members, 3);
    ASSERT_TRUE(members.empty());
}

static void test_file_save_load_roundtrip()
{
    auto path = tmp("roundtrip");
    std::vector<Member> original = {
        make_member(1, "홍길동", "hong@test.com"),
        make_member(2, "김철수", "kim@test.com"),
    };
    save_to(path, original);
    auto loaded = load_from(path);
    del(path);

    ASSERT_EQ(static_cast<int>(loaded.size()), 2);
    ASSERT_EQ(loaded[0].id, 1);
    ASSERT_STREQ(loaded[0].name, "홍길동");
    ASSERT_EQ(loaded[1].id, 2);
    ASSERT_STREQ(loaded[1].name, "김철수");
}

static void test_file_load_nonexistent()
{
    auto result = load_from(tmp("does_not_exist_xyz"));
    ASSERT_TRUE(result.empty());
}

static void test_file_save_load_empty_list()
{
    auto path = tmp("empty_list");
    save_to(path, {});
    auto loaded = load_from(path);
    del(path);
    ASSERT_TRUE(loaded.empty());
}

static void test_full_crud_scenario()
{
    auto path = tmp("full_crud");

    // Create
    std::vector<Member> members;
    members.push_back(make_member(next_id(members), "회원A"));
    members.push_back(make_member(next_id(members), "회원B"));
    members.push_back(make_member(next_id(members), "회원C"));
    save_to(path, members);

    // Read
    members = load_from(path);
    ASSERT_EQ(static_cast<int>(members.size()), 3);
    ASSERT_STREQ(find_by_id(members, 2)->name, "회원B");

    // Update
    Member u = make_member(2, "회원B_수정");
    update_member(members, 2, u);
    save_to(path, members);

    // Delete
    remove_by_id(members, 1);
    save_to(path, members);

    // Verify
    members = load_from(path);
    del(path);
    ASSERT_EQ(static_cast<int>(members.size()), 2);
    ASSERT_FALSE(find_by_id(members, 1).has_value());
    ASSERT_STREQ(find_by_id(members, 2)->name, "회원B_수정");
    ASSERT_TRUE(find_by_id(members, 3).has_value());
}

static void test_add_after_delete_no_id_conflict()
{
    std::vector<Member> members = { make_member(1), make_member(2), make_member(3) };
    remove_by_id(members, 3);
    // next_id는 현재 최대값(2) + 1 = 3
    int new_id = next_id(members);
    ASSERT_EQ(new_id, 3);
    members.push_back(make_member(new_id, "새회원"));
    ASSERT_TRUE(find_by_id(members, 3).has_value());
}

// ===============================================================
// [SAFETY TESTS] 극단적 입력
// ===============================================================

static void test_empty_string_fields()
{
    Member m = { 1, "", "", "", "", "" };
    Member restored = member_from_json(member_to_json(m));
    ASSERT_STREQ(restored.name,       "");
    ASSERT_STREQ(restored.email,      "");
    ASSERT_STREQ(restored.address,    "");
}

static void test_very_long_string()
{
    auto path = tmp("longstr");
    std::string long_name(50000, 'A');
    Member m = { 1, long_name, "", "", "", "" };
    std::vector<Member> members = { m };
    save_to(path, members);
    auto loaded = load_from(path);
    del(path);
    ASSERT_EQ(static_cast<int>(loaded[0].name.size()), 50000);
}

static void test_json_special_chars_in_fields()
{
    auto path = tmp("special");
    std::string tricky = R"(이름 "따옴표" \역슬래시\ <꺽쇠> &앰퍼샌드& /슬래시/)";
    Member m = { 1, tricky, tricky, tricky, tricky, tricky };
    save_to(path, { m });
    auto loaded = load_from(path);
    del(path);
    ASSERT_STREQ(loaded[0].name,    tricky);
    ASSERT_STREQ(loaded[0].address, tricky);
}

static void test_control_chars_in_fields()
{
    auto path = tmp("control");
    std::string with_ctrl = "줄바꿈\n탭\t캐리지리턴\r끝";
    Member m = { 1, with_ctrl, "", "", "", "" };
    save_to(path, { m });
    auto loaded = load_from(path);
    del(path);
    ASSERT_STREQ(loaded[0].name, with_ctrl);
}

static void test_korean_chars()
{
    auto path = tmp("korean");
    std::string name    = "가나다라마바사아자차카타파하";
    std::string address = "대한민국 서울특별시 종로구 청와대로 1";
    Member m = { 1, name, "", "", "", address };
    save_to(path, { m });
    auto loaded = load_from(path);
    del(path);
    ASSERT_STREQ(loaded[0].name,    name);
    ASSERT_STREQ(loaded[0].address, address);
}

static void test_unicode_emoji()
{
    auto path = tmp("emoji");
    std::string emoji_name = "홍길동😀🎉🔥";
    Member m = { 1, emoji_name, "", "", "", "" };
    save_to(path, { m });
    auto loaded = load_from(path);
    del(path);
    ASSERT_STREQ(loaded[0].name, emoji_name);
}

static void test_id_zero()
{
    std::vector<Member> members = { make_member(0, "ID제로") };
    auto found = find_by_id(members, 0);
    ASSERT_TRUE(found.has_value());
    ASSERT_STREQ(found->name, "ID제로");
}

static void test_id_negative()
{
    std::vector<Member> members = { make_member(-1, "음수ID") };
    auto found = find_by_id(members, -1);
    ASSERT_TRUE(found.has_value());
    ASSERT_STREQ(found->name, "음수ID");
}

static void test_id_max_int()
{
    auto path = tmp("maxint");
    Member m = { INT_MAX, "최대ID", "", "", "", "" };
    save_to(path, { m });
    auto loaded = load_from(path);
    del(path);
    ASSERT_EQ(loaded[0].id, INT_MAX);
}

static void test_duplicate_ids()
{
    // 중복 ID가 존재할 때 find는 첫 번째를 반환, remove는 모두 제거
    std::vector<Member> members = { make_member(1, "첫번째"), make_member(1, "두번째") };
    auto found = find_by_id(members, 1);
    ASSERT_TRUE(found.has_value());
    ASSERT_STREQ(found->name, "첫번째"); // 첫 번째 매칭

    remove_by_id(members, 1);
    ASSERT_TRUE(members.empty()); // 중복 ID는 모두 삭제됨
}

static void test_corrupted_json_file()
{
    auto path = tmp("corrupt");
    { std::ofstream ofs(path); ofs << "{ this is not valid JSON !!!"; }
    auto loaded = load_from(path);
    del(path);
    ASSERT_TRUE(loaded.empty());
}

static void test_empty_file()
{
    auto path = tmp("emptyfile");
    { std::ofstream ofs(path); } // 빈 파일 생성
    auto loaded = load_from(path);
    del(path);
    ASSERT_TRUE(loaded.empty());
}

static void test_json_object_not_array()
{
    // 배열이 아닌 JSON 객체는 빈 목록 반환
    auto path = tmp("notarray");
    { std::ofstream ofs(path); ofs << R"({"key": "value"})"; }
    auto loaded = load_from(path);
    del(path);
    ASSERT_TRUE(loaded.empty());
}

static void test_missing_fields_use_defaults()
{
    // id만 있고 나머지 필드 없음 → 빈 문자열 기본값
    json j = { {"id", 10} };
    Member m = member_from_json(j);
    ASSERT_EQ(m.id, 10);
    ASSERT_STREQ(m.name,       "");
    ASSERT_STREQ(m.email,      "");
    ASSERT_STREQ(m.phone,      "");
    ASSERT_STREQ(m.birth_date, "");
    ASSERT_STREQ(m.address,    "");
}

static void test_wrong_type_for_id()
{
    // id가 문자열인 경우 → 기본값 0 사용, 크래시 없음
    json j = { {"id", "abc"}, {"name", "테스트"} };
    Member m = member_from_json(j);
    ASSERT_EQ(m.id, 0);
    ASSERT_STREQ(m.name, "테스트");
}

static void test_null_string_fields()
{
    // null 필드 → 빈 문자열 기본값
    json j = { {"id", 1}, {"name", nullptr}, {"email", nullptr} };
    Member m = member_from_json(j);
    ASSERT_EQ(m.id, 1);
    ASSERT_STREQ(m.name,  "");
    ASSERT_STREQ(m.email, "");
}

static void test_malformed_record_skipped_in_file()
{
    // JSON 배열에 정상 레코드와 손상 레코드 혼재
    auto path = tmp("malformed_record");
    {
        std::ofstream ofs(path);
        ofs << R"([
            {"id": 1, "name": "정상회원", "email": "", "phone": "", "birth_date": "", "address": ""},
            "이건 객체가 아님",
            {"id": 3, "name": "정상회원2", "email": "", "phone": "", "birth_date": "", "address": ""}
        ])";
    }
    auto loaded = load_from(path);
    del(path);
    // 정상 레코드 2개만 로드
    ASSERT_EQ(static_cast<int>(loaded.size()), 2);
    ASSERT_STREQ(loaded[0].name, "정상회원");
    ASSERT_STREQ(loaded[1].name, "정상회원2");
}

static void test_large_member_count()
{
    auto path = tmp("large");
    std::vector<Member> members;
    for (int i = 1; i <= 1000; ++i)
        members.push_back(make_member(i, "회원" + std::to_string(i)));
    save_to(path, members);
    auto loaded = load_from(path);
    del(path);
    ASSERT_EQ(static_cast<int>(loaded.size()), 1000);
    ASSERT_EQ(loaded[999].id, 1000);
    ASSERT_STREQ(loaded[999].name, "회원1000");
}

static void test_sequential_update_and_delete()
{
    // 연속 수정·삭제 후 상태 일관성
    std::vector<Member> members;
    for (int i = 1; i <= 5; ++i)
        members.push_back(make_member(i, "회원" + std::to_string(i)));

    update_member(members, 3, make_member(3, "수정된3"));
    remove_by_id(members, 1);
    remove_by_id(members, 5);
    update_member(members, 2, make_member(2, "수정된2"));

    ASSERT_EQ(static_cast<int>(members.size()), 3);
    ASSERT_FALSE(find_by_id(members, 1).has_value());
    ASSERT_STREQ(find_by_id(members, 2)->name, "수정된2");
    ASSERT_STREQ(find_by_id(members, 3)->name, "수정된3");
    ASSERT_FALSE(find_by_id(members, 5).has_value());
}

// ===============================================================
// main
// ===============================================================
int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::cout << "===================================\n";
    std::cout << "  회원 정보 CRUD 단위 테스트\n";
    std::cout << "===================================\n\n";

    std::cout << "--- [UNIT] JSON 변환 ---\n";
    run_test("member_to_json",                   test_member_to_json);
    run_test("member_from_json",                 test_member_from_json);
    run_test("json_roundtrip",                   test_json_roundtrip);

    std::cout << "\n--- [UNIT] next_id ---\n";
    run_test("next_id_empty",                    test_next_id_empty);
    run_test("next_id_sequential",               test_next_id_sequential);
    run_test("next_id_with_gaps",                test_next_id_with_gaps);

    std::cout << "\n--- [UNIT] find_by_id ---\n";
    run_test("find_by_id_found",                 test_find_by_id_found);
    run_test("find_by_id_not_found",             test_find_by_id_not_found);
    run_test("find_by_id_empty_list",            test_find_by_id_empty_list);

    std::cout << "\n--- [UNIT] update_member ---\n";
    run_test("update_member_success",            test_update_member_success);
    run_test("update_member_not_found",          test_update_member_not_found);
    run_test("update_preserves_id",              test_update_preserves_id);
    run_test("update_all_fields",                test_update_all_fields);

    std::cout << "\n--- [UNIT] remove_by_id ---\n";
    run_test("remove_by_id_success",             test_remove_by_id_success);
    run_test("remove_by_id_not_found",           test_remove_by_id_not_found);
    run_test("remove_preserves_others",          test_remove_preserves_others);
    run_test("remove_all_members",               test_remove_all_members);

    std::cout << "\n--- [UNIT] 파일 I/O ---\n";
    run_test("file_save_load_roundtrip",         test_file_save_load_roundtrip);
    run_test("file_load_nonexistent",            test_file_load_nonexistent);
    run_test("file_save_load_empty_list",        test_file_save_load_empty_list);

    std::cout << "\n--- [UNIT] 시나리오 ---\n";
    run_test("full_crud_scenario",               test_full_crud_scenario);
    run_test("add_after_delete_no_id_conflict",  test_add_after_delete_no_id_conflict);
    run_test("sequential_update_and_delete",     test_sequential_update_and_delete);

    std::cout << "\n--- [SAFETY] 극단적 입력 ---\n";
    run_test("empty_string_fields",              test_empty_string_fields);
    run_test("very_long_string (50000자)",       test_very_long_string);
    run_test("json_special_chars",               test_json_special_chars_in_fields);
    run_test("control_chars_in_fields",          test_control_chars_in_fields);
    run_test("korean_chars",                     test_korean_chars);
    run_test("unicode_emoji",                    test_unicode_emoji);
    run_test("id_zero",                          test_id_zero);
    run_test("id_negative",                      test_id_negative);
    run_test("id_max_int (INT_MAX)",             test_id_max_int);
    run_test("duplicate_ids",                    test_duplicate_ids);
    run_test("corrupted_json_file",              test_corrupted_json_file);
    run_test("empty_file",                       test_empty_file);
    run_test("json_object_not_array",            test_json_object_not_array);
    run_test("missing_fields_use_defaults",      test_missing_fields_use_defaults);
    run_test("wrong_type_for_id",                test_wrong_type_for_id);
    run_test("null_string_fields",               test_null_string_fields);
    run_test("malformed_record_skipped",         test_malformed_record_skipped_in_file);
    run_test("large_member_count (1000명)",      test_large_member_count);

    std::cout << "\n===================================\n";
    std::cout << "  결과: " << s_passed << " 통과 / " << s_failed << " 실패\n";
    std::cout << "===================================\n";

    return s_failed > 0 ? 1 : 0;
}
