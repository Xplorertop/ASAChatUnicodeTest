#include "API/ARK/Ark.h"


#pragma comment(lib, "AsaApi.lib")


EChatType::Type GetChatTypeIntType(int ChatType)
{

    EChatType::Type chatEnum;
    switch (ChatType)
    {
    case 0:
        chatEnum = EChatType::GlobalChat;
        break;
    case 1:
        chatEnum = EChatType::ProximityChat;
        break;
    case 2:
        chatEnum = EChatType::GlobalTribeChat;
        break;
    case 3:
        chatEnum = EChatType::AllianceChat;
        break;
    default:
        break;
    }
    return chatEnum;
}
EChatSendMode::Type GetChatSendModeIntType(int SendMode)
{
    /*GlobalChat = 0x0,
    GlobalTribeChat = 0x1,
    LocalChat = 0x2,
    AllianceChat = 0x3,
    MAX = 0x4*/
    EChatSendMode::Type chatEnum;
    switch (SendMode)
    {
    case 0:
        chatEnum = EChatSendMode::GlobalChat;
        break;
    case 1:
        chatEnum = EChatSendMode::GlobalTribeChat;
        break;
    case 2:
        chatEnum = EChatSendMode::LocalChat;
        break;
    case 3:
        chatEnum = EChatSendMode::AllianceChat;
        break;
    default:
        break;
    }
    return chatEnum;
}


 

DECLARE_HOOK(AShooterPlayerController_ClientChatMessage, void, AShooterPlayerController*, FPrimalChatMessage*);
void Hook_AShooterPlayerController_ClientChatMessage(AShooterPlayerController* _this, FPrimalChatMessage* Chat)
{
    // overriding code here

    // call original function with overriden params
    AShooterPlayerController_ClientChatMessage_original(_this, Chat);
}

// Intercept Messages
bool ChatMessageCallback(AShooterPlayerController* _this, FString* Message, int SendMode, int SenderPlatform, bool spam_check, bool command_executed)
{
    // Player Dead
    if (AsaApi::IApiUtils::IsPlayerDead(_this)) return false;

    // Player Spectator
    if (_this->PlayerStateField().Get()->bIsSpectator().Get() == true) return false;

    if (spam_check || command_executed) return false;

    // tribe / local / ally
    if (SendMode == 1 || SendMode == 2 || SendMode == 3) return false;


    const auto& player_controllers = AsaApi::GetApiUtils().GetWorld()->PlayerControllerListField();

    FString senderName = AsaApi::GetApiUtils().GetCharacterName(_this);
    FString senderSteamName = AsaApi::GetApiUtils().GetSteamName(_this);
    FString tribename = static_cast<APrimalCharacter*>(_this->CharacterField().Get())->TribeNameField();
    //int tribe_id = AsaApi::GetApiUtils().GetTribeID(_this);


    FPrimalChatMessage msg;
    msg.SendMode = GetChatSendModeIntType(SendMode);
    msg.ChatType = GetChatTypeIntType(SendMode);
    msg.senderPlatform = SenderPlatform;
    msg.UserId = AsaApi::GetApiUtils().GetEOSIDFromController(_this); //required on console player chat
    msg.SenderTeamIndex = AsaApi::GetApiUtils().GetTribeID(_this);
    msg.SenderId = AsaApi::GetApiUtils().GetTribeID(_this);
    msg.ReceivedTime = -1.0f;


    std::string conv = (*Message).ToStringUTF8();

    FString convBack = FString::FromStringUTF8(fmt::format("<RichColor Color=\"1,0,1,1\">{}</>", conv));

    msg.Message = convBack;
    msg.SenderName = senderName;
    msg.SenderSteamName = senderSteamName;
    msg.SenderTribeName = tribename;

    // GLOBAL CHAT
    if (SendMode == 0)
    {

        // Loop to all players and send message
        for (TWeakObjectPtr<APlayerController> player_controller : player_controllers)
        {
            AShooterPlayerController* shooter_pc = static_cast<AShooterPlayerController*>(player_controller.Get());

            if (shooter_pc)
            {
                msg.Receiver = AsaApi::GetApiUtils().GetCharacterName(shooter_pc);
                shooter_pc->ClientChatMessage(msg);
            }
        }

        return true;
    }


    return false;
}






// Called when the server is ready, do post-"server ready" initialization here
void OnServerReady()
{
    Log::GetLog()->info("ASA Chat Unicode");

    AsaApi::GetHooks().SetHook("AShooterPlayerController.ClientChatMessage(FPrimalChatMessage)", &Hook_AShooterPlayerController_ClientChatMessage, &AShooterPlayerController_ClientChatMessage_original);

    AsaApi::GetCommands().AddOnChatMessageCallback("ChatMessageCallback", &ChatMessageCallback);


}

// Hook that triggers once when the server is ready
DECLARE_HOOK(AShooterGameMode_BeginPlay, void, AShooterGameMode*);
void Hook_AShooterGameMode_BeginPlay(AShooterGameMode* _this)
{
    AShooterGameMode_BeginPlay_original(_this);

    OnServerReady();
}

// Called by AsaApi when the plugin is loaded, do pre-"server ready" initialization here
extern "C" __declspec(dllexport) void Plugin_Init()
{
    Log::Get().Init(PROJECT_NAME);

    AsaApi::GetHooks().SetHook("AShooterGameMode.BeginPlay()", Hook_AShooterGameMode_BeginPlay,
        &AShooterGameMode_BeginPlay_original);

    if (AsaApi::GetApiUtils().GetStatus() == AsaApi::ServerStatus::Ready)
        OnServerReady();
}

// Called by AsaApi when the plugin is unloaded, do cleanup here
extern "C" __declspec(dllexport) void Plugin_Unload()
{
    AsaApi::GetHooks().DisableHook("AShooterGameMode.BeginPlay()", Hook_AShooterGameMode_BeginPlay);

    AsaApi::GetHooks().DisableHook("AShooterPlayerController.ClientChatMessage(FPrimalChatMessage)", &Hook_AShooterPlayerController_ClientChatMessage);

    AsaApi::GetCommands().RemoveOnChatMessageCallback("ChatMessageCallback");
}