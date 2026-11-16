// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <deque>
#include <filesystem>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "common/fs/path_util.h"
#include "common/httplib.h"
#include "common/nextendo.h"

namespace Nextendo = Common::Nextendo;

TEST_CASE("Nextendo: PKCE S256 matches RFC 7636 appendix B", "[common]") {
    REQUIRE(Nextendo::PkceChallenge("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk") ==
            "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM");

    const auto pkce = Nextendo::MakePkce();
    REQUIRE(pkce.has_value());
    REQUIRE(pkce->verifier.size() == 43);
    REQUIRE(pkce->challenge == Nextendo::PkceChallenge(pkce->verifier));
}

TEST_CASE("Nextendo: base64url has no padding", "[common]") {
    const auto encode = [](std::string_view text) {
        return Nextendo::Base64UrlEncode(
            std::span{reinterpret_cast<const u8*>(text.data()), text.size()});
    };
    REQUIRE(encode("f") == "Zg");
    REQUIRE(encode("fo") == "Zm8");
    REQUIRE(encode("foo") == "Zm9v");
    REQUIRE(encode("\xfb\xff") == "-_8");
}

TEST_CASE("Nextendo: the authorize URL carries the loopback redirect and PKCE", "[common]") {
    const std::string url = Nextendo::BuildAuthorizeUrl("nxc_test", "st", "ch", 47823);
    REQUIRE(url.starts_with("https://nextendo.network/oauth/authorize?response_type=code"));
    REQUIRE(url.find("&client_id=nxc_test") != std::string::npos);
    REQUIRE(url.find("&redirect_uri=http%3A%2F%2F127.0.0.1%3A47823%2Fcallback") !=
            std::string::npos);
    REQUIRE(url.find("&scope=identity%20game.matchmaking") != std::string::npos);
    REQUIRE(url.find("&state=st&code_challenge=ch&code_challenge_method=S256") !=
            std::string::npos);
}

TEST_CASE("Nextendo: callback parsing", "[common]") {
    using Params = std::multimap<std::string, std::string>;

    const auto ok = Nextendo::ParseCallback(Params{{"code", "abc"}, {"state", "s1"}}, "s1");
    REQUIRE(ok.matched);
    REQUIRE(ok.error.empty());
    REQUIRE(ok.code == "abc");

    const auto wrong_state =
        Nextendo::ParseCallback(Params{{"code", "abc"}, {"state", "other"}}, "s1");
    REQUIRE_FALSE(wrong_state.matched);
    REQUIRE(wrong_state.code.empty());

    // A stray local request with an error but not this sign-in's state must not end it.
    const auto stray = Nextendo::ParseCallback(Params{{"error", "access_denied"}}, "s1");
    REQUIRE_FALSE(stray.matched);

    const auto denied = Nextendo::ParseCallback(
        Params{{"error", "access_denied"}, {"error_description", "no"}, {"state", "s1"}}, "s1");
    REQUIRE(denied.matched);
    REQUIRE(denied.error == "access_denied: no");

    const auto no_code = Nextendo::ParseCallback(Params{{"state", "s1"}}, "s1");
    REQUIRE(no_code.code.empty());
    REQUIRE_FALSE(no_code.error.empty());
}

TEST_CASE("Nextendo: token responses", "[common]") {
    const auto emulator = Nextendo::ParseTokenResponse(
        R"({"token_type":"Bearer","nex_token":"nx2.a.b",)"
        R"("account":{"pid":1800000001,"username":"P","friend_code":"SW-1"}})");
    REQUIRE(emulator.has_value());
    REQUIRE(emulator->nex_token == "nx2.a.b");
    REQUIRE(emulator->pid == 1800000001);
    REQUIRE(emulator->username == "P");
    REQUIRE(emulator->friend_code == "SW-1");

    const auto third_party = Nextendo::ParseTokenResponse(
        R"({"access_token":"at","refresh_token":"rt","expires_in":3600})");
    REQUIRE(third_party.has_value());
    REQUIRE(third_party->nex_token.empty());
    REQUIRE(third_party->access_token == "at");
    REQUIRE(third_party->refresh_token == "rt");
    REQUIRE(third_party->fields.size() == 3);

    const auto wrong_types = Nextendo::ParseTokenResponse(
        R"({"access_token":"at","refresh_token":null,"nex_token":7,)"
        R"("account":{"pid":"42","username":null,"friend_code":[]}})");
    REQUIRE(wrong_types.has_value());
    REQUIRE(wrong_types->access_token == "at");
    REQUIRE(wrong_types->refresh_token.empty());
    REQUIRE(wrong_types->nex_token.empty());
    REQUIRE(wrong_types->pid == 42);
    REQUIRE(wrong_types->username.empty());
    REQUIRE(wrong_types->friend_code.empty());

    const auto game = Nextendo::ParseTokenResponse(
        R"({"nex_token":"nx2.g.s","pid":1800000002,"friend_code":"SW-2","username":"Q",)"
        R"("expires_in":86400})");
    REQUIRE(game.has_value());
    REQUIRE(game->nex_token == "nx2.g.s");
    REQUIRE(game->pid == 1800000002);
    REQUIRE(game->friend_code == "SW-2");
    REQUIRE(game->username == "Q");
    REQUIRE(game->expires_in == 86400);

    REQUIRE_FALSE(Nextendo::ParseTokenResponse("not json").has_value());
}

TEST_CASE("Nextendo: only invalid_grant ends the session", "[common]") {
    REQUIRE(Nextendo::RefreshTokenRejected(R"({"error":"invalid_grant"})"));
    REQUIRE(Nextendo::RefreshTokenRejected(
        R"({"error":"invalid_grant","error_description":"refresh token expired"})"));
    REQUIRE_FALSE(Nextendo::RefreshTokenRejected(R"({"error":"invalid_client"})"));
    REQUIRE_FALSE(Nextendo::RefreshTokenRejected(R"({"error":null})"));
    REQUIRE_FALSE(Nextendo::RefreshTokenRejected("<html>Bad Gateway</html>"));
}

TEST_CASE("Nextendo: online status reply", "[common]") {
    const auto refused = Nextendo::ParseOnlineStatus(R"({"allow":false,"reason":"unverified"})");
    REQUIRE(refused.has_value());
    REQUIRE_FALSE(refused->allow);
    REQUIRE(refused->reason == "unverified");

    const auto allowed = Nextendo::ParseOnlineStatus(R"({"allow":true,"reason":""})");
    REQUIRE(allowed.has_value());
    REQUIRE(allowed->allow);

    REQUIRE_FALSE(Nextendo::ParseOnlineStatus(R"({"error":"Session invalide"})").has_value());
    REQUIRE_FALSE(Nextendo::ParseOnlineStatus("not json").has_value());
}

namespace {

using namespace std::chrono_literals;

s64 Now() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

Nextendo::Account SignedIn(s64 game_token_left, s64 access_token_left) {
    return {.pid = 1,
            .username = "tester",
            .nex_token = "nx2.old",
            .access_token = "access.old",
            .refresh_token = "refresh.old",
            .access_expires = Now() + access_token_left,
            .nex_expires = Now() + game_token_left};
}

// Answers the API on 127.0.0.1 in place of nextendo.network. Each path replies with its queued
// answers in order and repeats the last one.
class FakeNextendo {
public:
    FakeNextendo() {
        directory = std::filesystem::temp_directory_path() /
                    fmt::format("lichii-nextendo-test-{}", std::random_device{}());
        std::filesystem::create_directories(directory);
        previous_config = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir);
        Common::FS::SetEdenPath(Common::FS::EdenPath::ConfigDir, directory);

        const auto handle = [this](const httplib::Request& request, httplib::Response& response) {
            std::scoped_lock lock{mutex};
            ++hits[request.path];
            authorization[request.path] = request.get_header_value("Authorization");
            forms[request.path] = request.params;
            auto& queue = replies[request.path];
            const Canned reply = queue.empty() ? Canned{404, "{}"} : queue.front();
            if (queue.size() > 1) {
                queue.pop_front();
            }
            response.status = reply.status;
            response.set_content(reply.body, "application/json");
        };
        server.Get(".*", handle);
        server.Post(".*", handle);
        port = server.bind_to_any_port("127.0.0.1");
        REQUIRE(port > 0);
        thread = std::thread{[this] { void(server.listen_after_bind()); }};
        server.wait_until_ready();
        Nextendo::SetApiBaseForTests(fmt::format("http://127.0.0.1:{}", port));
        Nextendo::ClearAccount();
    }

    ~FakeNextendo() {
        server.stop();
        thread.join();
        Nextendo::ClearAccount();
        Nextendo::SetApiBaseForTests(std::string{Nextendo::ApiBase});
        Common::FS::SetEdenPath(Common::FS::EdenPath::ConfigDir, previous_config);
        std::error_code ec;
        std::filesystem::remove_all(directory, ec);
    }

    void Answer(const std::string& path, int status, std::string body) {
        std::scoped_lock lock{mutex};
        replies[path].push_back({status, std::move(body)});
    }

    int Hits(const std::string& path) {
        std::scoped_lock lock{mutex};
        return hits[path];
    }

    int TotalHits() {
        std::scoped_lock lock{mutex};
        int total = 0;
        for (const auto& [path, count] : hits) {
            total += count;
        }
        return total;
    }

    std::string Authorization(const std::string& path) {
        std::scoped_lock lock{mutex};
        return authorization[path];
    }

    std::string Param(const std::string& path, const std::string& key) {
        std::scoped_lock lock{mutex};
        const auto& form = forms[path];
        const auto it = form.find(key);
        return it == form.end() ? std::string{} : it->second;
    }

private:
    struct Canned {
        int status;
        std::string body;
    };

    std::filesystem::path directory;
    std::filesystem::path previous_config;
    httplib::Server server;
    std::thread thread;
    int port{};
    std::mutex mutex;
    std::map<std::string, std::deque<Canned>> replies;
    std::map<std::string, int> hits;
    std::map<std::string, std::string> authorization;
    std::map<std::string, httplib::Params> forms;
};

constexpr const char* ClientId = "nxc_test";
constexpr const char* TokenPath = "/api/oauth/token";
constexpr const char* NexTokenPath = "/api/nex-token";
constexpr const char* NewGameToken = R"({"nex_token":"nx2.new","pid":1,"username":"tester",)"
                                     R"("friend_code":"SW-0000-0000-0000","expires_in":86400})";
constexpr const char* NewAccessToken =
    R"({"access_token":"access.new","refresh_token":"refresh.new","expires_in":3600})";

} // namespace

TEST_CASE("Nextendo: the game token is renewed ahead of its expiry", "[common]") {
    FakeNextendo fake;
    fake.Answer(NexTokenPath, 200, NewGameToken);

    Nextendo::SaveAccount(SignedIn(2 * 60 * 60, 30 * 60));
    REQUIRE(Nextendo::EnsureGameToken(ClientId)->nex_token == "nx2.old");
    REQUIRE(fake.TotalHits() == 0);

    Nextendo::SaveAccount(SignedIn(Nextendo::RenewAheadSeconds - 60, 30 * 60));
    const auto account = Nextendo::EnsureGameToken(ClientId);
    REQUIRE(account->nex_token == "nx2.new");
    REQUIRE(account->nex_expires >= Now() + 86400 - 5);
    REQUIRE(fake.Hits(NexTokenPath) == 1);
    REQUIRE(fake.Hits(TokenPath) == 0);
    REQUIRE(fake.Authorization(NexTokenPath) == "Bearer access.old");
}

TEST_CASE("Nextendo: an expired access token is refreshed before the game token", "[common]") {
    FakeNextendo fake;
    fake.Answer(TokenPath, 200, NewAccessToken);
    fake.Answer(NexTokenPath, 200, NewGameToken);

    Nextendo::SaveAccount(SignedIn(60, -10));
    const auto account = Nextendo::EnsureGameToken(ClientId);
    REQUIRE(account->access_token == "access.new");
    REQUIRE(account->refresh_token == "refresh.new");
    REQUIRE(account->nex_token == "nx2.new");
    REQUIRE(fake.Param(TokenPath, "grant_type") == "refresh_token");
    REQUIRE(fake.Authorization(NexTokenPath) == "Bearer access.new");
}

TEST_CASE("Nextendo: invalid_grant signs out instead of retrying", "[common]") {
    FakeNextendo fake;
    fake.Answer(TokenPath, 400, R"({"error":"invalid_grant"})");

    Nextendo::SaveAccount(SignedIn(60, -10));
    REQUIRE_FALSE(Nextendo::EnsureGameToken(ClientId).has_value());
    REQUIRE(Nextendo::LastSignOutReason() == Nextendo::SignOutReason::SessionExpired);
    REQUIRE_FALSE(Nextendo::EnsureGameToken(ClientId).has_value());
    REQUIRE(fake.TotalHits() == 1);
}

TEST_CASE("Nextendo: 403 asks for a sign-in with game.matchmaking", "[common]") {
    FakeNextendo fake;
    fake.Answer(NexTokenPath, 403, R"({"error":"insufficient_scope"})");

    Nextendo::SaveAccount(SignedIn(60, 30 * 60));
    REQUIRE_FALSE(Nextendo::EnsureGameToken(ClientId).has_value());
    REQUIRE(Nextendo::LastSignOutReason() == Nextendo::SignOutReason::MissingScope);
    REQUIRE(fake.Hits(TokenPath) == 0);
    REQUIRE(fake.Hits(NexTokenPath) == 1);
}

TEST_CASE("Nextendo: 401 refreshes the access token once", "[common]") {
    SECTION("and the new one works") {
        FakeNextendo fake;
        fake.Answer(NexTokenPath, 401, "{}");
        fake.Answer(NexTokenPath, 200, NewGameToken);
        fake.Answer(TokenPath, 200, NewAccessToken);

        Nextendo::SaveAccount(SignedIn(60, 30 * 60));
        REQUIRE(Nextendo::EnsureGameToken(ClientId)->nex_token == "nx2.new");
        REQUIRE(fake.Hits(TokenPath) == 1);
        REQUIRE(fake.Hits(NexTokenPath) == 2);
    }
    SECTION("and signs out when the new one is refused too") {
        FakeNextendo fake;
        fake.Answer(NexTokenPath, 401, "{}");
        fake.Answer(TokenPath, 200, NewAccessToken);

        Nextendo::SaveAccount(SignedIn(60, 30 * 60));
        REQUIRE_FALSE(Nextendo::EnsureGameToken(ClientId).has_value());
        REQUIRE(Nextendo::LastSignOutReason() == Nextendo::SignOutReason::SessionRevoked);
        REQUIRE(fake.Hits(TokenPath) == 1);
        REQUIRE(fake.Hits(NexTokenPath) == 2);
    }
}

TEST_CASE("Nextendo: a server error keeps the account and backs off", "[common]") {
    FakeNextendo fake;
    fake.Answer(NexTokenPath, 500, "{}");

    Nextendo::SaveAccount(SignedIn(60, 30 * 60));
    REQUIRE(Nextendo::EnsureGameToken(ClientId)->nex_token == "nx2.old");
    REQUIRE(Nextendo::EnsureGameToken(ClientId)->nex_token == "nx2.old");
    REQUIRE(fake.Hits(NexTokenPath) == 1);
}

TEST_CASE("Nextendo: session heartbeat", "[common]") {
    SECTION("a refused game token is renewed at the next check") {
        FakeNextendo fake;
        fake.Answer("/api/nex-session", 401, "{}");
        fake.Answer(NexTokenPath, 200, NewGameToken);

        Nextendo::SaveAccount(SignedIn(2 * 60 * 60, 30 * 60));
        REQUIRE(Nextendo::KeepSessionAlive(ClientId) == Nextendo::SignOutReason::None);
        REQUIRE(fake.Hits(NexTokenPath) == 0);
        REQUIRE(Nextendo::EnsureGameToken(ClientId)->nex_token == "nx2.new");
    }
    SECTION("a session ended on the site signs out") {
        FakeNextendo fake;
        fake.Answer("/api/nex-session", 200, "{}");
        fake.Answer("/api/session-status", 200, R"({"revoked":true})");

        Nextendo::SaveAccount(SignedIn(2 * 60 * 60, 30 * 60));
        REQUIRE(Nextendo::KeepSessionAlive(ClientId) == Nextendo::SignOutReason::SessionRevoked);
        REQUIRE(fake.Authorization("/api/nex-session") == "Bearer nx2.old");
        REQUIRE_FALSE(Nextendo::LoadAccount().has_value());
    }
    SECTION("a refresh refused during the heartbeat is reported") {
        FakeNextendo fake;
        fake.Answer(TokenPath, 400, R"({"error":"invalid_grant"})");

        Nextendo::SaveAccount(SignedIn(60, -10));
        REQUIRE(Nextendo::KeepSessionAlive(ClientId) == Nextendo::SignOutReason::SessionExpired);
    }
}

TEST_CASE("Nextendo: sign-out revokes the refresh token", "[common]") {
    FakeNextendo fake;
    fake.Answer("/api/oauth/revoke", 200, R"({"ok":true})");

    Nextendo::RevokeToken(ClientId, "refresh.old");
    REQUIRE(fake.Param("/api/oauth/revoke", "token") == "refresh.old");
}

TEST_CASE("Nextendo: requests stay within the per-minute budget", "[common]") {
    FakeNextendo fake;
    fake.Answer("/api/nex-session", 200, "{}");
    fake.Answer("/api/session-status", 200, R"({"revoked":false})");

    Nextendo::SaveAccount(SignedIn(2 * 60 * 60, 30 * 60));
    for (int i = 0; i < 12; ++i) {
        void(Nextendo::KeepSessionAlive(ClientId));
    }
    REQUIRE(fake.TotalHits() == static_cast<int>(Nextendo::RequestsPerMinute));
}

TEST_CASE("Nextendo: sign-in takes a free port when the callback port is busy", "[common]") {
    FakeNextendo fake;
    fake.Answer(TokenPath, 200, NewAccessToken);
    fake.Answer(NexTokenPath, 200, NewGameToken);
    fake.Answer("/api/online-status", 200, R"({"allow":false,"reason":"discord_unlinked"})");

    httplib::Server blocker;
    blocker.set_socket_options([](socket_t) {});
    const bool blocked = blocker.bind_to_port("127.0.0.1", Nextendo::CallbackPort);

    std::thread browser;
    std::string redirect_port;
    const auto open_url = [&](const std::string& url) {
        const auto field = [&url](std::string_view key) {
            const auto start = url.find(key) + key.size();
            return url.substr(start, url.find('&', start) - start);
        };
        const std::string redirect = field("&redirect_uri=http%3A%2F%2F127.0.0.1%3A");
        redirect_port = redirect.substr(0, redirect.find('%'));
        browser = std::thread{[port = std::stoi(redirect_port), state = field("&state=")] {
            httplib::Client client{"127.0.0.1", port};
            void(client.Get("/callback?code=abc&state=" + state));
        }};
    };
    const auto result = Nextendo::SignIn(ClientId, open_url, std::stop_token{});
    if (browser.joinable()) {
        browser.join();
    }

    INFO(result.error);
    REQUIRE(result.ok);
    if (blocked) {
        REQUIRE(redirect_port != std::to_string(Nextendo::CallbackPort));
    }
    REQUIRE(fake.Param(TokenPath, "redirect_uri") ==
            "http://127.0.0.1:" + redirect_port + "/callback");
    REQUIRE(result.account.nex_token == "nx2.new");
    REQUIRE(fake.Authorization("/api/online-status") == "Bearer nx2.new");
    REQUIRE(result.online_status.has_value());
    REQUIRE(result.online_status->reason == "discord_unlinked");
}
