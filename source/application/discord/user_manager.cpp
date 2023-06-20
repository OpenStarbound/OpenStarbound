#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "user_manager.h"

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

class UserEvents final {
public:
    static void OnCurrentUserUpdate(void* callbackData)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->UserManager();
        module.OnCurrentUserUpdate();
    }
};

IDiscordUserEvents UserManager::events_{
  &UserEvents::OnCurrentUserUpdate,
};

Result UserManager::GetCurrentUser(User* currentUser)
{
    if (!currentUser) {
        return Result::InternalError;
    }

    auto result =
      internal_->get_current_user(internal_, reinterpret_cast<DiscordUser*>(currentUser));
    return static_cast<Result>(result);
}

void UserManager::GetUser(UserId userId, std::function<void(Result, User const&)> callback)
{
    static auto wrapper = [](void* callbackData, EDiscordResult result, DiscordUser* user) -> void {
        std::unique_ptr<std::function<void(Result, User const&)>> cb(
          reinterpret_cast<std::function<void(Result, User const&)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(static_cast<Result>(result), *reinterpret_cast<User const*>(user));
    };
    std::unique_ptr<std::function<void(Result, User const&)>> cb{};
    cb.reset(new std::function<void(Result, User const&)>(std::move(callback)));
    internal_->get_user(internal_, userId, cb.release(), wrapper);
}

} // namespace discord
