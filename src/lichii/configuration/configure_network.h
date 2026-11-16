// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <stop_token>
#include <QWidget>
#include "common/nextendo.h"

namespace Ui {
class ConfigureNetwork;
}

class ConfigureNetwork : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureNetwork(const Core::System& system_, QWidget* parent = nullptr,
                              bool per_game = false);
    ~ConfigureNetwork() override;

    void ApplyConfiguration();

    // Why nextendo.network refuses online play, from GET /api/online-status's reason.
    static QString OnlineRefusal(const std::string& reason);

private:
    void changeEvent(QEvent*) override;
    void RetranslateUI();
    void SetConfiguration();
    void SignInToNextendo();
    void RenewNextendoToken();
    void UpdateNextendoStatus(const QString& error = {}, bool signed_in = false);
    void ShowNextendoFriends();
    void UpdateNextendoFriendButtons();
    // Runs a friends request in the background, then shows the friend list it leaves cached.
    void RunFriendsRequest(
        std::function<Common::Nextendo::SocialResult(const std::string& client_id)> request,
        const QString& done = {});
    void AnswerNextendoFriend(Common::Nextendo::FriendAction action);

    std::unique_ptr<Ui::ConfigureNetwork> ui;
    std::stop_source nextendo_stop;
    bool nextendo_signing_in{};
    std::optional<Common::Nextendo::OnlineStatus> nextendo_online;

    const Core::System& system;
    // The account is global, so a game's properties don't offer it.
    const bool per_game;
};
