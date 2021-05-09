//
// Copyright (c) 2016-2020 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/sml.hpp>
#include <cassert>
#include <confu_boost/confuBoost.hxx>
#include <confu_soci/convenienceFunctionForSoci.hxx>
#include <functional>
#include <game_01_shared_class/serialization.hxx>
#include <iostream>
#include <map>
#include <pipes/pipes.hpp>
#include <queue>
#include <variant>
namespace sml = boost::sml;

template <typename TypeToSend>
void
sendObject (std::deque<std::string> &msgToSend, TypeToSend const &typeToSend)
{
  msgToSend.push_back (confu_soci::typeNameWithOutNamespace (typeToSend) + '|' + confu_boost::toString (typeToSend));
}

struct draw
{
};

struct loginStateMachine
{
};
struct makeGameMachine
{
  std::string accountName;
};

struct logoutSuccess
{
};

struct lobby
{
};
struct createGameLobbyWaitForServer
{
};
struct createGameLobby
{
};
struct createGameLobbyError
{
  std::string message{};
};
struct gameLobbyData
{
};

struct Lobby
{
  std::string createGameLobbyName;
  std::string createGameLobbyPassword;
  std::string joinGameLobbyName;
  std::string joinGameLobbyPassword;
  bool createCreateGameLobbyClicked = false;
  bool createJoinGameLobbyClicked = false;
  bool logoutButtonClicked = false;
};
struct CreateGameLobbyWaitForServer
{
  std::string message{};
  bool backToLobbyClicked{};
};
struct CreateGameLobbyError
{
  std::string message{};
  bool backToLobbyClicked{};
};
struct CreateGameLobby
{
  std::string accountName{};
  std::string gameLobbyName{};
  int maxUserInGameLobby{};
  int maxUserInGameLobbyToSend{};
  std::vector<std::string> accountNamesInGameLobby{};
  bool sendMaxUserCountClicked = false;
  bool leaveGameLobby = false;
};

struct ChatData
{
  std::string
  selectChannelComboBoxName ()
  {
    return selectedChannelName.value_or ("Select Channel");
  }
  std::vector<std::string> const &
  messagesForChannel (std::string const &channel)
  {
    return channelMessages.at (channel);
  }

  std::vector<std::string>
  channelNames ()
  {
    auto result = std::vector<std::string>{};
    channelMessages >>= pipes::transform ([] (auto const &channelAndMessages) { return std::get<0> (channelAndMessages); }) >>= pipes::push_back (result);
    return result;
  }

  std::optional<std::string> selectedChannelName;
  std::string channelToJoin;
  std::string messageToSendToChannel;
  std::map<std::string, std::vector<std::string>> channelMessages{};
  bool joinChannelClicked = false;
  bool sendMessageClicked = false;
};

struct MakeGameMachineData
{
  ChatData chatData{};
  std::string accountName{};
  std::deque<std::string> messagesToSendToServer{};
};

struct MessagesToSendToServer
{
  std::deque<std::string> messagesToSendToServer{};
};

const auto drawLobby = [] (Lobby &lobby, MakeGameMachineData &makeGameMachineData) { std::cout << "drawLobby" << std::endl; };
const auto drawCreateGameLobbyWaitForServer = [] (CreateGameLobbyWaitForServer &createGameLobbyWaitForServer) { std::cout << "drawCreateGameLobbyWaitForServer" << std::endl; };
const auto drawCreateGameLobbyError = [] (CreateGameLobbyError &createGameLobbyError) { std::cout << "drawCreateGameLobbyError" << std::endl; };
const auto drawCreateGameLobby = [] (CreateGameLobby &createGameLobby, ChatData &chatData) { std::cout << "drawCreateGameLobby" << std::endl; };

const auto evalLobby = [] (Lobby &lobby, MessagesToSendToServer &messagesToSendToServer, sml::back::process<createGameLobbyWaitForServer> process_event) {
  if (lobby.logoutButtonClicked)
    {
      sendObject (messagesToSendToServer.messagesToSendToServer, shared_class::LogoutAccount{});
    }
  if (lobby.createCreateGameLobbyClicked && not lobby.createGameLobbyName.empty ())
    {
      sendObject (messagesToSendToServer.messagesToSendToServer, shared_class::CreateGameLobby{ .name = lobby.createGameLobbyName, .password = lobby.createGameLobbyPassword });
      process_event (createGameLobbyWaitForServer{});
    }
  if (lobby.createJoinGameLobbyClicked && not lobby.joinGameLobbyName.empty ())
    {
      sendObject (messagesToSendToServer.messagesToSendToServer, shared_class::JoinGameLobby{ .name = lobby.joinGameLobbyName, .password = lobby.joinGameLobbyPassword });
      process_event (createGameLobbyWaitForServer{});
    }
};

const auto evalCreateGameLobbyWaitForServer = [] (CreateGameLobbyWaitForServer &createGameLobbyWaitForServer, MessagesToSendToServer &messagesToSendToServer, sml::back::process<lobby> process_event) {
  if (createGameLobbyWaitForServer.backToLobbyClicked) process_event (lobby{});
};

const auto evalCreateGameLobbyError = [] (CreateGameLobbyError &createGameLobbyError, MessagesToSendToServer &messagesToSendToServer, sml::back::process<lobby> process_event) {
  if (createGameLobbyError.backToLobbyClicked) process_event (lobby{});
};

const auto evalCreateGameLobby = [] (CreateGameLobby &createGameLobby, MessagesToSendToServer &messagesToSendToServer, sml::back::process<lobby> process_event) {
  if (createGameLobby.leaveGameLobby)
    {
      sendObject (messagesToSendToServer.messagesToSendToServer, shared_class::LeaveGameLobby{});
      process_event (lobby{});
    }
  if (createGameLobby.sendMaxUserCountClicked)
    {
      sendObject (messagesToSendToServer.messagesToSendToServer, shared_class::SetMaxUserSizeInCreateGameLobby{ .createGameLobbyName = createGameLobby.gameLobbyName, .maxUserSize = static_cast<size_t> (createGameLobby.maxUserInGameLobby) });
      // TODO button should be greyed out while request runs
    }
};

const auto evalChat = [] (MakeGameMachineData &makeGameMachineData, MessagesToSendToServer &messagesToSendToServer) {
  if (makeGameMachineData.chatData.joinChannelClicked && not makeGameMachineData.chatData.channelToJoin.empty ())
    {
      sendObject (messagesToSendToServer.messagesToSendToServer, shared_class::JoinChannel{ .channel = makeGameMachineData.chatData.channelToJoin });
      makeGameMachineData.chatData.channelToJoin.clear ();
    }
  if (makeGameMachineData.chatData.sendMessageClicked && makeGameMachineData.chatData.selectedChannelName && not makeGameMachineData.chatData.selectedChannelName->empty () && not makeGameMachineData.chatData.messageToSendToChannel.empty ())
    {
      sendObject (messagesToSendToServer.messagesToSendToServer, shared_class::BroadCastMessage{ .channel = makeGameMachineData.chatData.selectedChannelName.value (), .message = makeGameMachineData.chatData.messageToSendToChannel });
      makeGameMachineData.chatData.messageToSendToChannel.clear ();
    }
};

struct MakeGameMachine
{
  auto
  operator() () const noexcept
  {
    using namespace sml;
    return make_transition_table (
        // clang-format off
        //TODO add statemachine which wraps this and deals with lobby events and with evalChat 
        //right now we have to write the same code again and again we can send an event and handle it in wrapper
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/      
* state<Lobby>                        + event<createGameLobbyWaitForServer>                                                                       = state<CreateGameLobbyWaitForServer>
, state<Lobby>                        + event<draw>                         /(drawLobby,evalLobby,evalChat)         
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/ 
, state<CreateGameLobbyWaitForServer> + event<createGameLobbyError>                                                                               = state<CreateGameLobbyError>
, state<CreateGameLobbyWaitForServer> + event<gameLobbyData>                                                                                      = state<CreateGameLobby>
, state<CreateGameLobbyWaitForServer> + event<lobby>                                                                                              = state<Lobby>
, state<CreateGameLobbyWaitForServer> + event<draw>                         /(drawCreateGameLobbyWaitForServer,evalCreateGameLobbyWaitForServer)         
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/ 
, state<CreateGameLobbyError>         + event<lobby>                                                                                              = state<Lobby>
, state<CreateGameLobbyError>         + event<draw>                         /(drawCreateGameLobbyError,evalCreateGameLobbyError)  
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
, state<CreateGameLobby>              + event<lobby>                                                                                              = state<Lobby>
, state<CreateGameLobby>              + event<draw>                         /(drawCreateGameLobby,evalCreateGameLobby,evalChat)  
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
);
    // clang-format on 
  }
};

struct login{};
struct loginWaitForServer{};
struct loginError{
  std::string message{};
};
struct loginSuccess{
  std::string accountName{};
};
struct createAccount{};
struct createAccountWaitForServer{};
struct createAccountError{
  std::string message{};
};
struct createAccountSuccess{};

struct Login{
  std::string accountName{};
  std::string password{};
  bool createAccountClicked=false;
  bool loginClicked=false;
};
struct LoginWaitForServer{
  std::string message{};
  bool backToLoginClicked{};
};
struct LoginError{
  std::string message{};
  bool backToLoginClicked{};
};
struct CreateAccount{
  std::string accountName{};
  std::string password{};
  bool createAccountClicked=false;
  bool backToLoginClicked{};
};
struct CreateAccountWaitForServer{
  std::string message{};
  bool backToAccountClicked{};
};
struct CreateAccountError{
  std::string message{};
  bool backToAccountClicked{};
};
struct CreateAccountSuccess{
  std::string message{};  
  bool backToAccountClicked{};
};

// TODO maybe we can use overloaded lambda and name it draw and overload it on the types to draw

const auto drawLogin = [] (Login &login) { std::cout<<"drawLogin"<<std::endl;};
const auto drawLoginWaitForServer = [] (LoginWaitForServer &loginWaitForServer) {std::cout<<"drawLoginWaitForServer"<<std::endl;};
const auto drawLoginError = [] (LoginError &loginError) {std::cout<<"drawLoginError"<<std::endl;};
const auto drawCreateAccount = [] (CreateAccount &createAccount) {std::cout<<"drawCreateAccount"<<std::endl;};
const auto drawCreateAccountWaitForServer = [] (CreateAccountWaitForServer &createAccountWaitForServer) {std::cout<<"drawCreateAccountWaitForServer"<<std::endl;};
const auto drawCreateAccountError = [] (CreateAccountError &createAccountError) {std::cout<<"drawCreateAccountError"<<std::endl;};
const auto drawCreateAccountSuccess = [] (CreateAccountSuccess &createAccountSuccess) {std::cout<<"drawCreateAccountSuccess"<<std::endl;};

auto evalLogin = [] (Login &login,MessagesToSendToServer &messagesToSendToServer,sml::back::process<loginWaitForServer,createAccount> process_event) {
  if(login.loginClicked) {
    sendObject (messagesToSendToServer.messagesToSendToServer, shared_class::LoginAccount{ .accountName = login.accountName, .password = login.password });
    process_event(loginWaitForServer{});
  }
  if(login.createAccountClicked)  process_event(createAccount{});
};

const auto evalLoginWaitForServer = [] (LoginWaitForServer &loginWaitForServer,sml::back::process<login> process_event) {
  if(loginWaitForServer.backToLoginClicked) process_event(login{});
};

const auto evalLoginError = [] (LoginError &loginError,sml::back::process<login> process_event) {
  if(loginError.backToLoginClicked) process_event(login{});
};

const auto evalCreateAccount = [] (CreateAccount &createAccount,MessagesToSendToServer &messagesToSendToServer,sml::back::process<createAccountWaitForServer,login> process_event) {
  if(createAccount.createAccountClicked) {
    sendObject (messagesToSendToServer.messagesToSendToServer, shared_class::CreateAccount{ .accountName = createAccount.accountName, .password = createAccount.password });
    process_event(createAccountWaitForServer{});
  }
  if(createAccount.backToLoginClicked)process_event(login{});
};

const auto evalCreateAccountWaitForServer = [] (CreateAccountWaitForServer &createAccountWaitForServer,sml::back::process<createAccount> process_event) {
  if(createAccountWaitForServer.backToAccountClicked) process_event(createAccount{});
};

const auto evalCreateAccountError = [] (CreateAccountError &createAccountError,sml::back::process<createAccount> process_event) {
  if(createAccountError.backToAccountClicked) process_event(createAccount{});
};
const auto evalCreateAccountSuccess = [] (CreateAccountSuccess &createAccountSuccess,sml::back::process<createAccount> process_event) {
  if(createAccountSuccess.backToAccountClicked) process_event(createAccount{});
};

 auto setAccountName = [] (loginSuccess const&loginSuccessEv,MakeGameMachineData &makeGameMachineData) {
 makeGameMachineData.accountName=loginSuccessEv.accountName;
};

struct LoginStateMachine
{
  auto
  operator() () const noexcept
  {
    using namespace sml;

      return make_transition_table(
        // clang-format off
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/      
* state<Login>                        + event<loginWaitForServer>                                                                           = state<LoginWaitForServer>
, state<Login>                        + event<createAccount>                                                                                = state<CreateAccount>
, state<Login>                        + event<draw>                        /(drawLogin,evalLogin)         
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/      
, state<LoginWaitForServer>           + event<loginError>                                                                                   = state<LoginError>
, state<LoginWaitForServer>           + event<loginSuccess>                / (setAccountName,process(makeGameMachine{}))                    = X      
, state<LoginWaitForServer>           + event<login>                                                                                        = state<Login>
, state<LoginWaitForServer>           + event<draw>                        /(drawLoginWaitForServer,evalLoginWaitForServer)         
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/      
, state<LoginError>                   + event<login>                                                                                        = state<Login>
, state<LoginError>                   + event<draw>                        /(drawLoginError,evalLoginError)     
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/      
, state<CreateAccount>                + event<createAccountWaitForServer>                                                                   = state<CreateAccountWaitForServer>
, state<CreateAccount>                + event<login>                                                                                        = state<Login>
, state<CreateAccount>                + event<draw>                        /(drawCreateAccount,evalCreateAccount)  
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/      
, state<CreateAccountWaitForServer>   + event<createAccountError>                                                                           = state<createAccountError>
, state<CreateAccountWaitForServer>   + event<createAccountSuccess>                                                                         = state<CreateAccountSuccess>
, state<CreateAccountWaitForServer>   + event<createAccount>                                                                                = state<CreateAccount>
, state<CreateAccountWaitForServer>   + event<draw>                        /(drawCreateAccountWaitForServer,evalCreateAccountWaitForServer)
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/      
, state<CreateAccountError>           + event<createAccount>                                                                                = state<CreateAccount>
, state<CreateAccountError>           + event<draw>                        /(drawCreateAccountError,evalCreateAccountError)
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/      
, state<CreateAccountSuccess>         + event<createAccount>                                                                                = state<CreateAccount>
, state<CreateAccountSuccess>         + event<draw>                        /(drawCreateAccountSuccess,evalCreateAccountSuccess)
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/      
);
    // clang-format on
  }
};

struct WrapperMachine
{

  auto
  operator() ()
  {
    using namespace sml;

    auto resetGameMachineData = [] (MakeGameMachineData &makeGameMachineData) { makeGameMachineData = MakeGameMachineData{}; };

    return make_transition_table (
        // clang-format off
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/      
* state<LoginStateMachine>            + event<makeGameMachine>                                             = state<MakeGameMachine>
, state<MakeGameMachine>              + event<logoutSuccess>                                               = state<LoginStateMachine>
, state<MakeGameMachine>              + sml::on_exit<makeGameMachine>            / resetGameMachineData
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/      
// clang-format on   
);

  }
};



// struct my_logger
// {
//   template <class SM, class TEvent>
//   void
//   log_process_event (const TEvent &)
//   {
//     // printf ("[%s][process_event] %s\n", sml::aux::get_type_name<SM> (), sml::aux::get_type_name<TEvent> ());
//   }

//   template <class SM, class TGuard, class TEvent>
//   void
//   log_guard (const TGuard &, const TEvent &, bool /*result*/)
//   {
//     // printf ("[%s][guard] %s %s %s\n", sml::aux::get_type_name<SM> (), sml::aux::get_type_name<TGuard> (), sml::aux::get_type_name<TEvent> (), (result ? "[OK]" : "[Reject]"));
//   }

//   template <class SM, class TAction, class TEvent>
//   void
//   log_action (const TAction &, const TEvent &)
//   {
//     // printf ("[%s][action] %s %s\n", sml::aux::get_type_name<SM> (), sml::aux::get_type_name<TAction> (), sml::aux::get_type_name<TEvent> ());
//   }

//   template <class SM, class TSrcState, class TDstState>
//   void
//   log_state_change (const TSrcState & /*src*/, const TDstState & /*dst*/)
//   {
//     //  printf ("[%s][transition] %s -> %s\n", sml::aux::get_type_name<SM> (), src.c_str (), dst.c_str ());
//   }
// };

int
main ()
{
  using namespace sml;
  //my_logger logger;
  auto makeGameMachineData = MakeGameMachineData{};
  auto messagesToSendToServer = MessagesToSendToServer{};
  //sml::sm<WrapperMachine, sml::logger<my_logger>, sml::process_queue<std::queue>> sm{ makeGameMachineData, messagesToSendToServer, logger };
  sml::sm<WrapperMachine,  sml::process_queue<std::queue>> sm{ makeGameMachineData, messagesToSendToServer};
  sm.process_event (loginWaitForServer{});
  sm.process_event (loginSuccess{ "my account name" });
  sm.process_event (createGameLobbyWaitForServer{});
  sm.process_event (gameLobbyData{});
  sm.process_event (lobby{});
  sm.process_event (draw{});
  sm.process_event (logoutSuccess{});
  // assert (sm.is (sml::X));
}