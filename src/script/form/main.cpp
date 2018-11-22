// Deps: out/script_form.so: out/script_base.so
#include "../base/main.h"

#include <StaticHook.h>
#include <api.h>

struct ModalFormRequestPacket : Packet {
  int id;
  std::string data;
  ModalFormRequestPacket(unsigned char playerSubIndex, int id, std::string data)
      : Packet(playerSubIndex)
      , id(id)
      , data(data) {}
  ~ModalFormRequestPacket() {}

  virtual void *getId() const;
  virtual void *getName() const;
  virtual void *write(BinaryStream &) const;
  virtual void *read(BinaryStream &);
  virtual void *handle(NetworkIdentifier const &, NetEventCallback &) const;
  virtual bool disallowBatching() const;
};

struct ModalFormResponsePacket : Packet {
  int id;
  std::string data;
  ModalFormResponsePacket(unsigned char playerSubIndex, int id, std::string data)
      : Packet(playerSubIndex)
      , id(id)
      , data(data) {}

  virtual void *getId() const;
  virtual void *getName() const;
  virtual void *write(BinaryStream &) const;
  virtual void *read(BinaryStream &);
  virtual void *handle(NetworkIdentifier const &, NetEventCallback &) const;
  virtual bool disallowBatching() const;
};

struct ServerSettingsResponsePacket : Packet {
  int id;
  std::string data;
  ServerSettingsResponsePacket(unsigned char playerSubIndex, int id, std::string data)
      : Packet(playerSubIndex)
      , id(id)
      , data(data) {}

  virtual void *getId() const;
  virtual void *getName() const;
  virtual void *write(BinaryStream &) const;
  virtual void *read(BinaryStream &);
  virtual void *handle(NetworkIdentifier const &, NetEventCallback &) const;
  virtual bool disallowBatching() const;
};

struct FixedFunction {
  int32_t rid;
  scm::callback<void, std::string> fun;
  FixedFunction(int32_t rid, scm::callback<void, std::string> fun)
      : rid(rid)
      , fun(fun) {
    scm_gc_protect_object(fun);
  }
  FixedFunction(FixedFunction &&rhs)
      : rid(rhs.rid)
      , fun(rhs.fun) {
    rhs.fun.setInvalid();
  }
  operator int32_t() { return rid; }
  void operator()(std::string val) { fun(val); }
  ~FixedFunction() {
    if (scm_is_true(fun)) scm_gc_unprotect_object(fun);
  }
};

std::unordered_map<std::string, FixedFunction> callbacks;

TInstanceHook(void, _ZN20ServerNetworkHandler23handleModalFormResponseERK17NetworkIdentifierRK23ModalFormResponsePacket, ServerNetworkHandler,
              NetworkIdentifier const &nid, ModalFormResponsePacket &packet) {
  CLog::info("SF","debug 1");
  auto it = callbacks.find(_getServerPlayer(nid, packet.playerSubIndex)->getXUID());
  if (it != callbacks.end()) {
    CLog::info("SF","debug 2 %d = %d", it->second.rid, packet.id);
    if (it->second.rid == packet.id) {
      it->second(packet.data);
      callbacks.erase(it);
    }
  }
}

MAKE_HOOK(server_settings, "open-server-settings", ServerPlayer *);
TInstanceHook(void, _ZN16NetEventCallback27handleServerSettingsRequestERK17NetworkIdentifierRK27ServerSettingsRequestPacket, ServerNetworkHandler,
              NetworkIdentifier const &nid, ModalFormResponsePacket &packet) {
  auto result = _getServerPlayer(nid, packet.playerSubIndex);
  if (result)
    server_settings(result);
  else
    Log::warn("form", "Player Not Found: %s", nid.toString().c_str());
}

SCM_DEFINE_PUBLIC(c_send_form, "send-form", 3, 0, 0,
                  (scm::val<ServerPlayer *> player, scm::val<std::string> request, scm::callback<void, std::string> callback),
                  "Send form to player") {
  int id = rand();
  CLog::info("SF","send-form generate id %d", id);
  ModalFormRequestPacket packet{ player->getClientSubId(), id, request.get() };
  auto it = callbacks.find(player->getXUID());
  if (it != callbacks.end()) callbacks.erase(it); //先删除再添加
  callbacks.emplace(player->getXUID(), FixedFunction{ id, callback });
  player->sendNetworkPacket(packet);
  return SCM_UNSPECIFIED;
}

SCM_DEFINE_PUBLIC(c_send_server_settings_form, "send-settings-form", 3, 0, 0,
                  (scm::val<ServerPlayer *> player, scm::val<std::string> request, scm::callback<void, std::string> callback),
                  "Send settings form to player") {
  int id = rand();
  CLog::info("SF","settings_form generate id %d", id);
  ServerSettingsResponsePacket packet{ player->getClientSubId(), id, request.get() };
  auto it = callbacks.find(player->getXUID());
  if (it != callbacks.end()) callbacks.erase(it); //先删除再添加
  callbacks.emplace(player->getXUID(), FixedFunction{ id, callback });
  player->sendNetworkPacket(packet);
  return SCM_UNSPECIFIED;
}

PRELOAD_MODULE("minecraft form") {
#ifndef DIAG
#include "main.x"
#endif
}
