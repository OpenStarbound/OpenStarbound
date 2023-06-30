#ifndef _DISCORD_GAME_SDK_H_
#define _DISCORD_GAME_SDK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define DISCORD_VERSION 1
#define DISCORD_APPLICATION_VERSION 1
#define DISCORD_USERS_VERSION 1
#define DISCORD_IMAGES_VERSION 1
#define DISCORD_ACTIVITIES_VERSION 1
#define DISCORD_RELATIONSHIPS_VERSION 1
#define DISCORD_LOBBIES_VERSION 1
#define DISCORD_NETWORK_VERSION 1
#define DISCORD_OVERLAY_VERSION 1
#define DISCORD_STORAGE_VERSION 1

enum EDiscordResult {
    DiscordResult_Ok,
    DiscordResult_ServiceUnavailable,
    DiscordResult_InvalidVersion,
    DiscordResult_LockFailed,
    DiscordResult_InternalError,
    DiscordResult_InvalidPaylaod,
    DiscordResult_InvalidCommand,
    DiscordResult_InvalidPermissions,
    DiscordResult_NotFetched,
    DiscordResult_NotFound,
    DiscordResult_Conflict,
    DiscordResult_InvalidSecret,
    DiscordResult_InvalidJoinSecret,
    DiscordResult_NoEligibleActivity,
    DiscordResult_InvalidInvite,
    DiscordResult_NotAuthenticated,
    DiscordResult_InvalidAccessToken,
    DiscordResult_ApplicationMismatch,
    DiscordResult_InvalidDataUrl,
    DiscordResult_InvalidBase64,
    DiscordResult_NotFiltered,
    DiscordResult_LobbyFull,
    DiscordResult_InvalidLobbySecret,
    DiscordResult_InvalidFilename,
    DiscordResult_InvalidFileSize,
    DiscordResult_InvalidEntitlement,
    DiscordResult_NotInstalled,
    DiscordResult_NotRunning,
};

enum EDiscordCreateFlags {
    DiscordCreateFlags_Default = 0,
    DiscordCreateFlags_NoRequireDiscord = 1,
};

enum EDiscordLogLevel {
    DiscordLogLevel_Error = 1,
    DiscordLogLevel_Warn,
    DiscordLogLevel_Info,
    DiscordLogLevel_Debug,
};

enum EDiscordImageType {
    DiscordImageType_User,
};

enum EDiscordActivityType {
    DiscordActivityType_Playing,
    DiscordActivityType_Streaming,
    DiscordActivityType_Listening,
    DiscordActivityType_Watching,
};

enum EDiscordActivityActionType {
    DiscordActivityActionType_Join = 1,
    DiscordActivityActionType_Spectate,
};

enum EDiscordActivityJoinRequestReply {
    DiscordActivityJoinRequestReply_No,
    DiscordActivityJoinRequestReply_Yes,
    DiscordActivityJoinRequestReply_Ignore,
};

enum EDiscordStatus {
    DiscordStatus_Offline = 0,
    DiscordStatus_Online = 1,
    DiscordStatus_Idle = 2,
    DiscordStatus_DoNotDisturb = 4,
};

enum EDiscordRelationshipType {
    DiscordRelationshipType_None,
    DiscordRelationshipType_Friend,
    DiscordRelationshipType_Blocked,
    DiscordRelationshipType_PendingIncoming,
    DiscordRelationshipType_PendingOutgoing,
    DiscordRelationshipType_Implicit,
};

enum EDiscordLobbyType {
    DiscordLobbyType_Private = 1,
    DiscordLobbyType_Public,
};

enum EDiscordLobbySearchComparison {
    DiscordLobbySearchComparison_LessThanOrEqual = -2,
    DiscordLobbySearchComparison_LessThan,
    DiscordLobbySearchComparison_Equal,
    DiscordLobbySearchComparison_GreaterThan,
    DiscordLobbySearchComparison_GreaterThanOrEqual,
    DiscordLobbySearchComparison_NotEqual,
};

enum EDiscordLobbySearchCast {
    DiscordLobbySearchCast_String = 1,
    DiscordLobbySearchCast_Number,
};

typedef int64_t DiscordClientId;
typedef int32_t DiscordVersion;
typedef int64_t DiscordSnowflake;
typedef int64_t DiscordTimestamp;
typedef DiscordSnowflake DiscordUserId;
typedef char DiscordUserName[256];
typedef char DiscordUserAvatar[128];
typedef char DiscordUserDiscriminator[8];
typedef char DiscordAccessToken[128];
typedef char DiscordOAuth2Scopes[1024];
typedef char DiscordLocale[128];
typedef char DiscordBranch[4096];
typedef uint8_t* DiscordImageData;
typedef char DiscordImageId[128];
typedef char DiscordImageCaption[128];
typedef char DiscordPartyId[128];
typedef int32_t DiscordPartyMemberCount;
typedef char DiscordSecret[128];
typedef DiscordSnowflake DiscordLobbyId;
typedef char DiscordLobbySecret[128];
typedef char DiscordMetadataKey[256];
typedef char DiscordMetadataValue[4096];
typedef uint8_t* DiscordLobbyData;
typedef uint64_t DiscordNetworkSessionId;
typedef uint8_t DiscordNetworkChannelId;
typedef uint8_t* DiscordNetworkData;
typedef char DiscordStorageFileName[260];
typedef uint8_t* DiscordStorageData;

struct DiscordUser {
    DiscordUserId id;
    DiscordUserName username;
    DiscordUserDiscriminator discriminator;
    DiscordUserAvatar avatar;
    int bot;
};

struct DiscordOAuth2Token {
    DiscordAccessToken access_token;
    DiscordOAuth2Scopes scopes;
    DiscordTimestamp expires;
};

struct DiscordImageHandle {
    enum EDiscordImageType type;
    int64_t id;
    uint32_t size;
};

struct DiscordImageDimensions {
    uint32_t width;
    uint32_t height;
};

struct DiscordActivityTimestamps {
    DiscordTimestamp start;
    DiscordTimestamp end;
};

struct DiscordActivityAssets {
    DiscordImageId large_image;
    DiscordImageCaption large_text;
    DiscordImageId small_image;
    DiscordImageCaption small_text;
};

struct DiscordPartySize {
    DiscordPartyMemberCount current_size;
    DiscordPartyMemberCount max_size;
};

struct DiscordActivityParty {
    DiscordPartyId id;
    struct DiscordPartySize size;
};

struct DiscordActivitySecrets {
    DiscordSecret match;
    DiscordSecret join;
    DiscordSecret spectate;
};

struct DiscordActivity {
    enum EDiscordActivityType type;
    char name[128];
    char state[128];
    char details[128];
    struct DiscordActivityTimestamps timestamps;
    struct DiscordActivityAssets assets;
    struct DiscordActivityParty party;
    struct DiscordActivitySecrets secrets;
    int instance;
};

struct DiscordPresence {
    enum EDiscordStatus status;
    struct DiscordActivity activity;
};

struct DiscordRelationship {
    enum EDiscordRelationshipType type;
    struct DiscordUser user;
    struct DiscordPresence presence;
};

struct DiscordLobby {
    DiscordLobbyId id;
    enum EDiscordLobbyType type;
    DiscordSnowflake owner_id;
    DiscordLobbySecret secret;
    uint32_t capacity;
};

struct DiscordFileStat {
    DiscordStorageFileName filename;
    uint64_t size;
    uint64_t last_modified;
};

struct IDiscordLobbyTransaction {
    enum EDiscordResult (*set_type)(struct IDiscordLobbyTransaction* lobby_transaction, enum EDiscordLobbyType type);
    enum EDiscordResult (*set_owner)(struct IDiscordLobbyTransaction* lobby_transaction, DiscordSnowflake owner_id);
    enum EDiscordResult (*set_capacity)(struct IDiscordLobbyTransaction* lobby_transaction, uint32_t capacity);
    enum EDiscordResult (*set_metadata)(struct IDiscordLobbyTransaction* lobby_transaction, DiscordMetadataKey key, DiscordMetadataValue value);
    enum EDiscordResult (*delete_metadata)(struct IDiscordLobbyTransaction* lobby_transaction, DiscordMetadataKey key);
};

struct IDiscordLobbyMemberTransaction {
    enum EDiscordResult (*set_metadata)(struct IDiscordLobbyMemberTransaction* lobby_member_transaction, DiscordMetadataKey key, DiscordMetadataValue value);
    enum EDiscordResult (*delete_metadata)(struct IDiscordLobbyMemberTransaction* lobby_member_transaction, DiscordMetadataKey key);
};

struct IDiscordLobbySearch {
    enum EDiscordResult (*filter)(struct IDiscordLobbySearch* lobby_search, DiscordMetadataKey key, enum EDiscordLobbySearchComparison comparison, enum EDiscordLobbySearchCast cast, DiscordMetadataValue value);
    enum EDiscordResult (*sort)(struct IDiscordLobbySearch* lobby_search, DiscordMetadataKey key, enum EDiscordLobbySearchCast cast, DiscordMetadataValue value);
    enum EDiscordResult (*limit)(struct IDiscordLobbySearch* lobby_search, uint32_t limit);
};

typedef void IDiscordApplicationEvents;

struct IDiscordApplication {
    enum EDiscordResult (*destroy)(struct IDiscordApplication* application);
    enum EDiscordResult (*validate_or_exit)(struct IDiscordApplication* application, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
    enum EDiscordResult (*get_current_locale)(struct IDiscordApplication* application, DiscordLocale* locale);
    enum EDiscordResult (*get_current_branch)(struct IDiscordApplication* application, DiscordBranch* branch);
    enum EDiscordResult (*get_oauth2_token)(struct IDiscordApplication* application, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result, struct DiscordOAuth2Token* oauth2_token));
};

struct IDiscordUsersEvents {
    void (*on_current_user_update)(void* event_data);
};

struct IDiscordUsers {
    enum EDiscordResult (*destroy)(struct IDiscordUsers* users);
    enum EDiscordResult (*get_current_user)(struct IDiscordUsers* users, struct DiscordUser* current_user);
    enum EDiscordResult (*fetch)(struct IDiscordUsers* users, DiscordUserId user_id, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result, struct DiscordUser* user));
};

typedef void IDiscordImagesEvents;

struct IDiscordImages {
    enum EDiscordResult (*destroy)(struct IDiscordImages* images);
    enum EDiscordResult (*fetch)(struct IDiscordImages* images, struct DiscordImageHandle handle, int refresh, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result, struct DiscordImageHandle handle_result));
    enum EDiscordResult (*get_dimensions)(struct IDiscordImages* images, struct DiscordImageHandle handle, struct DiscordImageDimensions* dimensions);
    enum EDiscordResult (*get_data)(struct IDiscordImages* images, struct DiscordImageHandle handle, DiscordImageData data, uint32_t data_length);
};

struct IDiscordActivitiesEvents {
    void (*on_activity_join)(void* event_data, DiscordSecret secret);
    void (*on_activity_spectate)(void* event_data, DiscordSecret secret);
    void (*on_activity_join_request)(void* event_data, struct DiscordUser* user);
    void (*on_activity_invite)(void* event_data, enum EDiscordActivityActionType type, struct DiscordUser* user, struct DiscordActivity* activity);
};

struct IDiscordActivities {
    enum EDiscordResult (*destroy)(struct IDiscordActivities* activities);
    enum EDiscordResult (*register_)(struct IDiscordActivities* activities, const char* command);
    enum EDiscordResult (*register_steam)(struct IDiscordActivities* activities, uint32_t steam_id);
    enum EDiscordResult (*update_activity)(struct IDiscordActivities* activities, struct DiscordActivity* activity, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
    enum EDiscordResult (*clear_activity)(struct IDiscordActivities* activities, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
    enum EDiscordResult (*respond)(struct IDiscordActivities* activities, DiscordUserId user_id, enum EDiscordActivityJoinRequestReply reply, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
    enum EDiscordResult (*invite_user)(struct IDiscordActivities* activities, DiscordUserId user_id, enum EDiscordActivityActionType type, const char* content, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
    enum EDiscordResult (*accept_invite)(struct IDiscordActivities* activities, DiscordUserId user_id, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
};

struct IDiscordRelationshipsEvents {
    void (*on_relationships_update)(void* event_data);
    void (*on_relationship_update)(void* event_data, struct DiscordRelationship* relationship);
};

struct IDiscordRelationships {
    enum EDiscordResult (*destroy)(struct IDiscordRelationships* relationships);
    enum EDiscordResult (*filter)(struct IDiscordRelationships* relationships, void* filter_data, int (*filter)(void* filter_data, struct DiscordRelationship* relationship));
    enum EDiscordResult (*count)(struct IDiscordRelationships* relationships, int32_t* count);
    enum EDiscordResult (*get)(struct IDiscordRelationships* relationships, DiscordUserId user_id, struct DiscordRelationship* relationship);
    enum EDiscordResult (*at)(struct IDiscordRelationships* relationships, uint32_t index, struct DiscordRelationship* relationship);
};

struct IDiscordLobbiesEvents {
    void (*on_lobby_update)(void* event_data, int64_t lobby_id);
    void (*on_lobby_delete)(void* event_data, int64_t lobby_id, uint32_t reason);
    void (*on_lobby_member_connect)(void* event_data, int64_t lobby_id, int64_t user_id);
    void (*on_lobby_member_update)(void* event_data, int64_t lobby_id, int64_t user_id);
    void (*on_lobby_member_disconnect)(void* event_data, int64_t lobby_id, int64_t user_id);
    void (*on_lobby_message)(void* event_data, int64_t lobby_id, int64_t user_id, DiscordLobbyData data, uint32_t data_length);
    void (*on_lobby_speaking)(void* event_data, int64_t lobby_id, int64_t user_id, int speaking);
};

struct IDiscordLobbies {
    enum EDiscordResult (*destroy)(struct IDiscordLobbies* lobbies);
    enum EDiscordResult (*create_lobby_transaction)(struct IDiscordLobbies* lobbies, struct IDiscordLobbyTransaction** transaction);
    enum EDiscordResult (*get_lobby_transaction)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, struct IDiscordLobbyTransaction** transaction);
    enum EDiscordResult (*get_member_transaction)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, DiscordUserId user_id, struct IDiscordLobbyMemberTransaction** transaction);
    enum EDiscordResult (*create_lobby)(struct IDiscordLobbies* lobbies, struct IDiscordLobbyTransaction* transaction, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result, struct DiscordLobby* lobby));
    enum EDiscordResult (*update_lobby)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, struct IDiscordLobbyTransaction* transaction, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
    enum EDiscordResult (*delete_lobby)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
    enum EDiscordResult (*connect)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, DiscordLobbySecret lobby_secret, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result, struct DiscordLobby* lobby));
    enum EDiscordResult (*disconnect)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
    enum EDiscordResult (*get_lobby)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, struct DiscordLobby* lobby);
    enum EDiscordResult (*get_lobby_metadata)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, DiscordMetadataKey key, DiscordMetadataValue* value);
    enum EDiscordResult (*get_member_count)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, int32_t* count);
    enum EDiscordResult (*get_member_user_id)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, int32_t index, DiscordUserId* user_id);
    enum EDiscordResult (*get_member_user)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, DiscordUserId user_id, struct DiscordUser* user);
    enum EDiscordResult (*get_member_metadata)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, DiscordUserId user_id, DiscordMetadataKey key, DiscordMetadataValue* value);
    enum EDiscordResult (*update_member)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, DiscordUserId user_id, struct IDiscordLobbyMemberTransaction* transaction, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
    enum EDiscordResult (*send)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, DiscordLobbyData data, uint32_t data_length);
    enum EDiscordResult (*create_lobby_search)(struct IDiscordLobbies* lobbies, struct IDiscordLobbySearch** lobby_search);
    enum EDiscordResult (*search)(struct IDiscordLobbies* lobbies, struct IDiscordLobbySearch* query, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
    enum EDiscordResult (*get_lobby_count)(struct IDiscordLobbies* lobbies, int32_t* count);
    enum EDiscordResult (*get_lobby_id)(struct IDiscordLobbies* lobbies, int32_t index, DiscordLobbyId* lobby_id);
    enum EDiscordResult (*voice_connect)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
    enum EDiscordResult (*voice_disconnect)(struct IDiscordLobbies* lobbies, DiscordLobbyId lobby_id, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
};

struct IDiscordNetworkEvents {
    void (*on_message)(void* event_data, DiscordNetworkSessionId from, DiscordNetworkChannelId channel, DiscordNetworkData data, uint32_t data_length);
};

struct IDiscordNetwork {
    enum EDiscordResult (*destroy)(struct IDiscordNetwork* network);
    enum EDiscordResult (*get_session_id)(struct IDiscordNetwork* network, DiscordNetworkSessionId* session_id);
    enum EDiscordResult (*flush)(struct IDiscordNetwork* network);
    enum EDiscordResult (*open_channel)(struct IDiscordNetwork* network, DiscordNetworkSessionId remote, DiscordNetworkChannelId channel);
    enum EDiscordResult (*open_reliable_channel)(struct IDiscordNetwork* network, DiscordNetworkSessionId remote, DiscordNetworkChannelId channel);
    enum EDiscordResult (*send)(struct IDiscordNetwork* network, DiscordNetworkSessionId remote, DiscordNetworkChannelId channel, DiscordNetworkData data, uint32_t data_length);
    enum EDiscordResult (*close_channel)(struct IDiscordNetwork* network, DiscordNetworkSessionId remote, DiscordNetworkChannelId channel);
};

struct IDiscordOverlayEvents {
    void (*on_overlay_locked)(void* event_data, int locked);
};

struct IDiscordOverlay {
    enum EDiscordResult (*destroy)(struct IDiscordOverlay* overlay);
    enum EDiscordResult (*is_enabled)(struct IDiscordOverlay* overlay, int* enabled);
    enum EDiscordResult (*is_locked)(struct IDiscordOverlay* overlay, int* locked);
    enum EDiscordResult (*set_locked)(struct IDiscordOverlay* overlay, int locked);
    enum EDiscordResult (*open_activity_invite)(struct IDiscordOverlay* overlay, enum EDiscordActivityActionType type, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
};

typedef void IDiscordStorageEvents;

struct IDiscordStorage {
    enum EDiscordResult (*destroy)(struct IDiscordStorage* storage);
    enum EDiscordResult (*read)(struct IDiscordStorage* storage, DiscordStorageFileName name, DiscordStorageData data, uint32_t data_length, uint32_t* read);
    enum EDiscordResult (*read_async)(struct IDiscordStorage* storage, DiscordStorageFileName name, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result, DiscordStorageData data, uint32_t data_length));
    enum EDiscordResult (*read_async_partial)(struct IDiscordStorage* storage, DiscordStorageFileName name, uint64_t offset, uint64_t length, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result, DiscordStorageData data, uint32_t data_length));
    enum EDiscordResult (*write)(struct IDiscordStorage* storage, DiscordStorageFileName name, DiscordStorageData data, uint32_t data_length);
    enum EDiscordResult (*write_async)(struct IDiscordStorage* storage, DiscordStorageFileName name, DiscordStorageData data, uint32_t data_length, void* callback_data, void (*callback)(void* callback_data, enum EDiscordResult result));
    enum EDiscordResult (*delete_)(struct IDiscordStorage* storage, DiscordStorageFileName name);
    enum EDiscordResult (*exists)(struct IDiscordStorage* storage, DiscordStorageFileName name, int* exists);
    enum EDiscordResult (*count)(struct IDiscordStorage* storage, int32_t* count);
    enum EDiscordResult (*stat)(struct IDiscordStorage* storage, DiscordStorageFileName name, struct DiscordFileStat* stat);
    enum EDiscordResult (*stat_index)(struct IDiscordStorage* storage, int32_t index, struct DiscordFileStat* stat);
};

struct IDiscordCoreEvents {
    void (*on_ready)(void* event_data);
};

struct IDiscordCore {
    enum EDiscordResult (*destroy)(struct IDiscordCore* core);
    enum EDiscordResult (*run_callbacks)(struct IDiscordCore* core);
    enum EDiscordResult (*set_log_hook)(struct IDiscordCore* core, enum EDiscordLogLevel min_level, void* hook_data, void (*hook)(void* hook_data, enum EDiscordLogLevel level, const char* message));
    enum EDiscordResult (*create_application)(struct IDiscordCore* core, DiscordVersion version, struct IDiscordApplicationEvents* events, void* event_data, struct IDiscordApplication** result);
    enum EDiscordResult (*create_users)(struct IDiscordCore* core, DiscordVersion version, struct IDiscordUsersEvents* events, void* event_data, struct IDiscordUsers** result);
    enum EDiscordResult (*create_images)(struct IDiscordCore* core, DiscordVersion version, struct IDiscordImagesEvents* events, void* event_data, struct IDiscordImages** result);
    enum EDiscordResult (*create_activities)(struct IDiscordCore* core, DiscordVersion version, struct IDiscordActivitiesEvents* events, void* event_data, struct IDiscordActivities** result);
    enum EDiscordResult (*create_relationships)(struct IDiscordCore* core, DiscordVersion version, struct IDiscordRelationshipsEvents* events, void* event_data, struct IDiscordRelationships** result);
    enum EDiscordResult (*create_lobbies)(struct IDiscordCore* core, DiscordVersion version, struct IDiscordLobbiesEvents* events, void* event_data, struct IDiscordLobbies** result);
    enum EDiscordResult (*create_network)(struct IDiscordCore* core, DiscordVersion version, struct IDiscordNetworkEvents* events, void* event_data, struct IDiscordNetwork** result);
    enum EDiscordResult (*create_overlay)(struct IDiscordCore* core, DiscordVersion version, struct IDiscordOverlayEvents* events, void* event_data, struct IDiscordOverlay** result);
    enum EDiscordResult (*create_storage)(struct IDiscordCore* core, DiscordVersion version, struct IDiscordStorageEvents* events, void* event_data, struct IDiscordStorage** result);
};

struct DiscordCreateParams {
    DiscordClientId client_id;
    uint64_t flags;
    struct IDiscordCoreEvents* events;
    void* event_data;
};

enum EDiscordResult DiscordCreate(DiscordVersion version, struct DiscordCreateParams* params, struct IDiscordCore** result);

#ifdef __cplusplus
}
#endif

#endif