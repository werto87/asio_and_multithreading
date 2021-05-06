#include <algorithm>
#include <boost/mpl/list.hpp>
#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/transition.hpp>
#include <confu_boost/confuBoost.hxx>
#include <confu_soci/convenienceFunctionForSoci.hxx>
#include <deque>
#include <exception>
#include <game_01_shared_class/serialization.hxx>
#include <iostream>
#include <stdexcept>
#include <variant>

namespace sc = boost::statechart;
namespace mpl = boost::mpl;

template <class... Ts> struct overloaded : Ts...
{
  using Ts::operator()...;
};
template <class... Ts> overloaded (Ts...) -> overloaded<Ts...>;

template <typename TypeToSend>
static void
sendObject (std::deque<std::string> &msgToSend, TypeToSend const &typeToSend)
{
  msgToSend.push_back (confu_soci::typeNameWithOutNamespace (typeToSend) + '|' + confu_boost::toString (typeToSend));
}

struct Login;
struct CreateAccount;
struct Lobby;
struct GameLobbyError;
struct CreateGameLobby;
struct JoinGameLobby;
struct LoginWaitForServer;
struct LoginAccountError;
struct MakeGameLobby;
struct CreateAccountError;
struct CreateAccountSuccess;
struct JoinChannelError;
struct GameLobby;
struct CreateAccountWaitForServer;
struct GameLobbyWaitForServer;
struct LoginWantToRelog;

struct EvCreateAccountSuccess : sc::event<EvCreateAccountSuccess>
{
};
struct EvCreateAccountError : sc::event<EvCreateAccountError>
{
  std::string error{};
};
struct EvLoginAccountSuccess : sc::event<EvLoginAccountSuccess>
{
};
struct EvLoginAccountError : sc::event<EvLoginAccountError>
{
  std::string error{};
};
struct EvLogoutAccountSuccess : sc::event<EvLogoutAccountSuccess>
{
};
struct EvJoinChannelSuccess : sc::event<EvJoinChannelSuccess>
{
  std::string channelName{};
};
struct EvJoinChannelError : sc::event<EvJoinChannelError>
{
  std::string error{};
};
struct EvMessage : sc::event<EvMessage>
{
  std::string channel{};
  std::string message{};
};
// Not handled
// struct EvBroadCastMessageSuccess : sc::event< EvBroadCastMessageSuccess>{};
// Not handled
// struct EvBroadCastMessageError : sc::event<EvBroadCastMessageError>
// {
//   std::string error{};
// };
// Not handled
// struct EvCreateGameLobbySuccess : sc::event< EvCreateGameLobbySuccess>{};

// Not handled
struct EvCreateGameLobbyError : sc::event<EvCreateGameLobbyError>
{
  std::string error{};
};
struct EvJoinGameLobbyError : sc::event<EvJoinGameLobbyError>
{
  std::string error{};
};

// struct EvJoinGameLobbySuccess : sc::event<EvJoinGameLobbySuccess>
// {
// };

// Not handled
// struct EvJoinGameLobbyError : sc::event< EvJoinGameLobbyError>{
//     std::string error{};
// };

struct EvUsersInGameLobby : sc::event<EvUsersInGameLobby>
{
  std::optional<std::string> gameLobbyName{};
  size_t maxUserInGameLobby{};
  std::vector<std::string> accountNamesInGameLobby{};
};
struct EvMaxUserSizeInCreateGameLobby : sc::event<EvMaxUserSizeInCreateGameLobby>
{
  size_t maxUserInGameLobby{};
};
// Not handled
// struct EvSetMaxUserSizeInCreateGameLobbyError : sc::event< EvSetMaxUserSizeInCreateGameLobbyError>{
//     std::string error{};
// };

struct EvLeaveGameLobbySuccess : sc::event<EvLeaveGameLobbySuccess>
{
};
struct EvLeaveGameLobbyError : sc::event<EvLeaveGameLobbyError>
{
  std::string error{};
};
struct EvWantToRelog : sc::event<EvWantToRelog>
{
  std::string destination{};
};
struct EvRelogToError : sc::event<EvRelogToError>
{
  std::string error{};
};

// checks state and calcs next state from it
struct EvNextState : sc::event<EvNextState>
{
};

struct LoginState
{
  std::string accountName;
  std::string password;
  bool createAccountButtonClicked = false;
  bool signInButtonClicked = false;
};

struct WaitForServerPopupState
{
  bool cancelButtonClicked;
};

struct MessageBoxPopupState
{
  std::string message;
  std::map<std::string, bool> buttons;
};

struct CreateAccountState
{
  std::string accountName;
  std::string password;
  bool createAccountClicked = false;
};

struct LobbyState
{
  std::string createGameLobbyName;
  std::string createGameLobbyPassword;
  std::string joinGameLobbyName;
  std::string joinGameLobbyPassword;
  bool createCreateGameLobbyClicked = false;
  bool createJoinGameLobbyClicked = false;
};
struct GameLobbyState
{
  std::optional<std::string> gameLobbyName{};
  size_t maxUserInGameLobby{};
  std::vector<std::string> accountNamesInGameLobby{};
};

typedef std::variant<LoginState, MessageBoxPopupState, CreateAccountState, LobbyState, GameLobbyState, WaitForServerPopupState> GuiState;

struct Machine : sc::state_machine<Machine, Login>
{
  GuiState state{};
  std::deque<std::string> msgToSend;
};

struct Login : sc::state<Login, Machine>
{
  typedef mpl::list<sc::custom_reaction<EvNextState>> reactions;
  Login (my_context ctx) : my_base (ctx) { outermost_context ().state = LoginState{}; }
  sc::result
  react (const EvNextState &)
  {
    auto const &loginState = std::get<LoginState> (outermost_context ().state);
    if (loginState.createAccountButtonClicked)
      {
        return transit<CreateAccount> ();
      }
    else if (loginState.signInButtonClicked && not loginState.accountName.empty () && not loginState.password.empty ())
      {
        sendObject (outermost_context ().msgToSend, shared_class::LoginAccount{ .accountName = loginState.accountName, .password = loginState.password });
        return transit<LoginWaitForServer> ();
      }
    else
      {
        return discard_event ();
      }
  }
};

struct LoginWaitForServer : sc::state<LoginWaitForServer, Machine>
{
  typedef mpl::list<sc::custom_reaction<EvNextState>, sc::transition<EvLoginAccountSuccess, Lobby>, sc::custom_reaction<EvLoginAccountError>, sc::custom_reaction<EvWantToRelog>> reactions;
  LoginWaitForServer (my_context ctx) : my_base (ctx) { outermost_context ().state = WaitForServerPopupState{}; }
  sc::result
  react (const EvNextState &)
  {
    if (std::get<WaitForServerPopupState> (outermost_context ().state).cancelButtonClicked)
      {
        return transit<Login> ();
      }
    else
      {
        return discard_event ();
      }
  }
  sc::result
  react (const EvLoginAccountError &evLoginAccountError)
  {
    outermost_context ().state = MessageBoxPopupState{ .message = evLoginAccountError.error, .buttons{ { "back", false } } };
    return transit<LoginAccountError> ();
  }
  sc::result
  react (const EvWantToRelog &evWantToRelog)
  {
    outermost_context ().state = MessageBoxPopupState{ .message = "do you want to go to lobby or relog to " + evWantToRelog.destination, .buttons{ { "Lobby", false }, { evWantToRelog.destination, false } } };
    return transit<LoginWantToRelog> ();
  }
};

struct LoginAccountError : sc::simple_state<LoginAccountError, Machine>
{
  typedef mpl::list<sc::custom_reaction<EvNextState>> reactions;
  sc::result
  react (const EvNextState &)
  {
    auto messageBoxPopupState = std::get<MessageBoxPopupState> (outermost_context ().state);
    if (messageBoxPopupState.buttons.count ("back") == 1 && messageBoxPopupState.buttons.find ("back")->second)
      {
        return transit<Login> ();
      }
    else
      {
        return discard_event ();
      }
  }
};

struct LoginWantToRelog : sc::simple_state<LoginWantToRelog, Machine>
{
  typedef mpl::list<sc::custom_reaction<EvNextState>> reactions;
  sc::result
  react (const EvNextState &)
  {
    auto messageBoxPopupState = std::get<MessageBoxPopupState> (outermost_context ().state);
    if (messageBoxPopupState.buttons.count ("Lobby") == 1 && messageBoxPopupState.buttons.find ("Lobby")->second)
      {
        return transit<Lobby> ();
      }
    else if (messageBoxPopupState.buttons.count ("Game Lobby") == 1 && messageBoxPopupState.buttons.find ("Game Lobby")->second)
      {
        return transit<GameLobby> ();
      }
    else
      {
        return discard_event ();
      }
  }
};

struct CreateAccount : sc::state<CreateAccount, Machine>
{
  typedef mpl::list<sc::custom_reaction<EvNextState>> reactions;
  CreateAccount (my_context ctx) : my_base (ctx) { outermost_context ().state = CreateAccountState{}; }
  sc::result
  react (const EvNextState &)
  {
    auto const &createAccountState = std::get<CreateAccountState> (outermost_context ().state);
    if (createAccountState.createAccountClicked)
      {
        sendObject (outermost_context ().msgToSend, shared_class::CreateAccount{ .accountName = createAccountState.accountName, .password = createAccountState.password });
        return transit<CreateAccountWaitForServer> ();
      }
    else
      {
        return discard_event ();
      }
  }
};

struct CreateAccountWaitForServer : sc::state<CreateAccountWaitForServer, Machine>
{
  typedef mpl::list<sc::custom_reaction<EvNextState>, sc::custom_reaction<EvCreateAccountSuccess>, sc::custom_reaction<EvCreateAccountError>> reactions;
  CreateAccountWaitForServer (my_context ctx) : my_base (ctx) { outermost_context ().state = WaitForServerPopupState{}; }
  sc::result
  react (const EvNextState &)
  {
    if (std::get<WaitForServerPopupState> (outermost_context ().state).cancelButtonClicked)
      {
        return transit<CreateAccount> ();
      }
    else
      {
        return discard_event ();
      }
  }
  sc::result
  react (const EvCreateAccountSuccess &)
  {
    outermost_context ().state = MessageBoxPopupState{ .message = "create account success", .buttons{ { "back", false } } };
    return transit<CreateAccountSuccess> ();
  }
  sc::result
  react (const EvCreateAccountError &evCreateAccountError)
  {
    outermost_context ().state = MessageBoxPopupState{ .message = evCreateAccountError.error, .buttons{ { "back", false } } };
    return transit<CreateAccountError> ();
  }
};

struct CreateAccountError : sc::simple_state<CreateAccountError, Machine>
{
  typedef mpl::list<sc::custom_reaction<EvNextState>> reactions;
  sc::result
  react (const EvNextState &)
  {
    auto messageBoxPopupState = std::get<MessageBoxPopupState> (outermost_context ().state);
    if (messageBoxPopupState.buttons.count ("back") == 1 && messageBoxPopupState.buttons.find ("back")->second)
      {
        return transit<CreateAccount> ();
      }
    else
      {
        return discard_event ();
      }
  }
};

struct CreateAccountSuccess : sc::simple_state<CreateAccountSuccess, Machine>
{
  typedef mpl::list<sc::custom_reaction<EvNextState>> reactions;
  sc::result
  react (const EvNextState &)
  {
    auto messageBoxPopupState = std::get<MessageBoxPopupState> (outermost_context ().state);
    if (messageBoxPopupState.buttons.count ("back") == 1 && messageBoxPopupState.buttons.find ("back")->second)
      {
        return transit<CreateAccount> ();
      }
    else
      {
        return discard_event ();
      }
  }
};

struct ChatState
{
  std::optional<std::string> selectedChannelName;
  std::string channelToJoin;
  std::string messageToSendToChannel;
  std::map<std::string, std::vector<std::string>> channelMessages{};
};
struct MakeGameLobby : sc::simple_state<MakeGameLobby, Machine, Lobby>
{
  typedef mpl::list<sc::custom_reaction<EvMessage>, sc::transition<EvLogoutAccountSuccess, Login>, sc::custom_reaction<EvJoinChannelSuccess>, sc::custom_reaction<EvJoinChannelSuccess>, sc::custom_reaction<EvJoinChannelError>> reactions;

  sc::result
  react (const EvJoinChannelSuccess &evJoinChannelSuccess)
  {
    chatState.channelMessages.insert_or_assign (evJoinChannelSuccess.channelName, std::vector<std::string>{});
    return discard_event ();
  }
  sc::result
  react (const EvJoinChannelError &evJoinChannelError)
  {
    outermost_context ().state = MessageBoxPopupState{ .message = evJoinChannelError.error, .buttons{ { "back", false } } };
    return transit<JoinChannelError> ();
  }
  sc::result
  react (const EvMessage &evMessage)
  {
    if (chatState.channelMessages.count (evMessage.channel) == 1)
      {
        chatState.channelMessages.find (evMessage.channel)->second.push_back (evMessage.message);
      }
    else
      {
        std::cout << "can not find chat channel: " << evMessage.channel << "on client" << std::endl;
      }
    return discard_event ();
  }
  ChatState chatState{};
};

struct JoinChannelError : sc::simple_state<JoinChannelError, Machine>
{
  typedef mpl::list<sc::custom_reaction<EvNextState>> reactions;
  sc::result
  react (const EvNextState &)
  {
    auto messageBoxPopupState = std::get<MessageBoxPopupState> (outermost_context ().state);
    if (messageBoxPopupState.buttons.count ("back") == 1 && messageBoxPopupState.buttons.find ("back")->second)
      {
        return transit<Lobby> ();
      }
    else
      {
        return discard_event ();
      }
  }
};

struct Lobby : sc::state<Lobby, MakeGameLobby>
{
  typedef mpl::list<sc::custom_reaction<EvNextState>> reactions;

  Lobby (my_context ctx) : my_base (ctx) { outermost_context ().state = LobbyState{}; }
  sc::result
  react (const EvNextState &)
  {
    auto lobbyState = std::get<LobbyState> (outermost_context ().state);
    if (lobbyState.createCreateGameLobbyClicked && not lobbyState.createGameLobbyName.empty ())
      {
        sendObject (outermost_context ().msgToSend, shared_class::CreateGameLobby{ .name = lobbyState.createGameLobbyName, .password = lobbyState.createGameLobbyPassword });
        return transit<GameLobbyWaitForServer> ();
      }
    else if (lobbyState.createJoinGameLobbyClicked && not lobbyState.joinGameLobbyName.empty ())
      {
        sendObject (outermost_context ().msgToSend, shared_class::JoinGameLobby{ .name = lobbyState.joinGameLobbyName, .password = lobbyState.joinGameLobbyPassword });
        return transit<GameLobbyWaitForServer> ();
      }
    else
      {
        return discard_event ();
      }
  }
};

struct GameLobbyWaitForServer : sc::state<GameLobbyWaitForServer, Machine>
{

  typedef mpl::list<sc::custom_reaction<EvNextState>, sc::custom_reaction<EvJoinGameLobbyError>, sc::custom_reaction<EvUsersInGameLobby>, sc::custom_reaction<EvCreateGameLobbyError>> reactions;
  GameLobbyWaitForServer (my_context ctx) : my_base (ctx) { outermost_context ().state = WaitForServerPopupState{}; }
  sc::result
  react (const EvNextState &)
  {
    if (std::get<WaitForServerPopupState> (outermost_context ().state).cancelButtonClicked)
      {
        return transit<Lobby> ();
      }
    else
      {
        return discard_event ();
      }
  }
  sc::result
  react (const EvJoinGameLobbyError &evJoinGameLobbyError)
  {
    outermost_context ().state = MessageBoxPopupState{ .message = evJoinGameLobbyError.error, .buttons{ { "back", false } } };
    return transit<GameLobbyError> ();
  }
  sc::result
  react (const EvCreateGameLobbyError &evCreateGameLobbyError)
  {
    outermost_context ().state = MessageBoxPopupState{ .message = evCreateGameLobbyError.error, .buttons{ { "back", false } } };
    return transit<GameLobbyError> ();
  }
  sc::result
  react (const EvUsersInGameLobby &evUsersInGameLobby)
  {
    outermost_context ().state = GameLobbyState{ .gameLobbyName = evUsersInGameLobby.gameLobbyName, .maxUserInGameLobby = evUsersInGameLobby.maxUserInGameLobby, .accountNamesInGameLobby = evUsersInGameLobby.accountNamesInGameLobby };
    return transit<GameLobby> ();
  }
};

struct GameLobby : sc::simple_state<GameLobby, MakeGameLobby>
{
  typedef mpl::list<sc::custom_reaction<EvNextState>> reactions;
  sc::result
  react (const EvNextState &)
  {
    return discard_event ();
  }
};

struct GameLobbyError : sc::simple_state<GameLobbyError, Machine>
{
  typedef mpl::list<sc::custom_reaction<EvNextState>> reactions;
  sc::result
  react (const EvNextState &)
  {
    auto messageBoxPopupState = std::get<MessageBoxPopupState> (outermost_context ().state);
    if (messageBoxPopupState.buttons.count ("back") == 1 && messageBoxPopupState.buttons.find ("back")->second)
      {
        return transit<Lobby> ();
      }
    else
      {
        return discard_event ();
      }
  }
};

void
visitAndAdvance (Machine &myMachine)
{
  auto visit = overloaded{
    [&] (LoginState &loginState) {
      //
      std::cout << "LoginState" << std::endl;
    },
    [&] (MessageBoxPopupState &messageBoxPopupState) {
      //
      std::cout << "MessageBoxPopupState" << std::endl;
    },
    [&] (CreateAccountState &createAccountState) {
      //
      std::cout << "CreateAccountState" << std::endl;
    },
    [&] (LobbyState &lobbyState) {
      //
      std::cout << "LobbyState" << std::endl;
    },
    [&] (GameLobbyState &gameLobbyState) {
      //
      std::cout << "GameLobbyState" << std::endl;
    },
    [&] (WaitForServerPopupState &waitForServerPopupState) {
      //
      std::cout << "WaitForServerPopupState" << std::endl;
    },
  };
  std::visit (visit, myMachine.state);
  myMachine.process_event (EvNextState{});
}

int
main ()
{
  Machine myMachine{};
  myMachine.initiate ();
  visitAndAdvance (myMachine);
  visitAndAdvance (myMachine);
  myMachine.process_event (EvLoginAccountSuccess{});
  visitAndAdvance (myMachine);
  myMachine.process_event (EvLogoutAccountSuccess{});
  visitAndAdvance (myMachine);
  return 0;
}
