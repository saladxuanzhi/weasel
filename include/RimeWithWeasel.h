#pragma once
#include <P2cEngine.h>
#include <WeaselIPC.h>
#include <WeaselUI.h>
#include <map>
#include <string>
#include <mutex>

#include <rime_api.h>

struct CaseInsensitiveCompare {
  bool operator()(const std::string& str1, const std::string& str2) const {
    std::string str1Lower, str2Lower;
    std::transform(str1.begin(), str1.end(), std::back_inserter(str1Lower),
                   [](char c) { return std::tolower(c); });
    std::transform(str2.begin(), str2.end(), std::back_inserter(str2Lower),
                   [](char c) { return std::tolower(c); });
    return str1Lower < str2Lower;
  }
};

typedef std::map<std::string, bool> AppOptions;
typedef std::map<std::string, AppOptions, CaseInsensitiveCompare>
    AppOptionsByAppName;

struct SessionStatus {
  SessionStatus() : style(weasel::UIStyle()), __synced(false), session_id(0) {
    RIME_STRUCT(RimeStatus, status);
  }
  weasel::UIStyle style;
  RimeStatus status;
  bool __synced;
  RimeSessionId session_id;
};
typedef std::map<DWORD, SessionStatus> SessionStatusMap;
typedef DWORD WeaselSessionId;

void LoadWeaselUIStyle(RimeConfig* config,
                       weasel::UIStyle& style,
                       bool initialize = true,
                       const std::string& color_scheme = std::string());

class RimeWithWeaselHandler : public weasel::RequestHandler {
 public:
  RimeWithWeaselHandler(weasel::UI* ui);
  virtual ~RimeWithWeaselHandler();
  virtual void Initialize();
  virtual void Finalize();
  virtual DWORD FindSession(WeaselSessionId ipc_id);
  virtual DWORD AddSession(LPWSTR buffer, EatLine eat = 0);
  virtual DWORD RemoveSession(WeaselSessionId ipc_id);
  virtual BOOL ProcessKeyEvent(weasel::KeyEvent keyEvent,
                               WeaselSessionId ipc_id,
                               EatLine eat);
  virtual void CommitComposition(WeaselSessionId ipc_id);
  virtual void ClearComposition(WeaselSessionId ipc_id);
  virtual void SelectCandidateOnCurrentPage(size_t index,
                                            WeaselSessionId ipc_id);
  virtual bool HighlightCandidateOnCurrentPage(size_t index,
                                               WeaselSessionId ipc_id,
                                               EatLine eat);
  virtual bool ChangePage(bool backward, WeaselSessionId ipc_id, EatLine eat);
  virtual void FocusIn(DWORD param, WeaselSessionId ipc_id);
  virtual void FocusOut(DWORD param, WeaselSessionId ipc_id);
  virtual void UpdateInputPosition(RECT const& rc, WeaselSessionId ipc_id);
  virtual void StartMaintenance();
  virtual void EndMaintenance();
  virtual void SetOption(WeaselSessionId ipc_id,
                         const std::string& opt,
                         bool val);
  virtual void UpdateColorTheme(BOOL darkMode);
  virtual void SetUIRefreshCallback(std::function<void()> callback);
  virtual void FetchContext(WeaselSessionId ipc_id, EatLine eat);

  void OnUpdateUI(std::function<void()> const& cb);
  // 异步模型结果就绪后，由服务端消息线程调用，重画当前会话的候选窗口。
  void RefreshUI();

 private:
  void _Setup();
  bool _IsDeployerRunning();
  void _UpdateUI(WeaselSessionId ipc_id);
  void _LoadSchemaSpecificSettings(WeaselSessionId ipc_id,
                                   const std::string& schema_id);
  void _LoadAppInlinePreeditSet(WeaselSessionId ipc_id,
                                bool ignore_app_name = false);
  bool _ShowMessage(weasel::Context& ctx, weasel::Status& status);
  bool _Respond(WeaselSessionId ipc_id, EatLine eat, bool include_commit = true);
  void _ReadClientInfo(WeaselSessionId ipc_id, LPWSTR buffer);
  void _GetCandidateInfo(weasel::CandidateInfo& cinfo, RimeContext& ctx);
  void _GetStatus(weasel::Status& stat,
                  WeaselSessionId ipc_id,
                  weasel::Context& ctx);
  void _GetContext(weasel::Context& ctx,
                   RimeSessionId session_id,
                   WeaselSessionId ipc_id);
  void _UpdateShowNotifications(RimeConfig* config, bool initialize = false);

  void _UpdateInlinePreeditStatus(WeaselSessionId ipc_id);

  // --- 含章·通变整句模型（P2C）---
  // 每个会话的注入状态：整句候选插在候选列表最前面一条。
  struct P2cSession {
    bool active = false;    // 结果已就绪、正在注入
    int highlight = 0;      // 视觉高亮（0 = 模型给的那句）
    std::string keys;       // 结果对应的输入
    std::wstring top_text;  // 模型给的首选，选中时直接上屏
    std::wstring commit_text;  // 待上屏，_Respond 会取走
  };

  void _LoadP2cSettings(RimeConfig* config);
  void _P2cSchedule(WeaselSessionId ipc_id);
  void _P2cClear(WeaselSessionId ipc_id);
  void _P2cInvalidate(WeaselSessionId ipc_id);
  bool _P2cSelectionKey(const weasel::KeyEvent& keyEvent, WeaselSessionId ipc_id);
  void _P2cInject(WeaselSessionId ipc_id, weasel::CandidateInfo& cinfo);
  // 当前输入是否正等着模型的结果（决定要不要告诉 TSF 过一会儿再来取）
  bool _P2cWaiting(WeaselSessionId ipc_id);
  // 取走待上屏的模型句子，没有就返回空
  std::wstring _P2cTakeCommit(WeaselSessionId ipc_id);
  // 这段输入值不值得问模型；顺手把分隔符去掉
  bool _P2cCurrentKeys(WeaselSessionId ipc_id, std::string* keys);
  static void OnP2cReady(uintptr_t session_key, void* context);

  RimeSessionId to_session_id(WeaselSessionId ipc_id) {
    return m_session_status_map[ipc_id].session_id;
  }
  SessionStatus& get_session_status(WeaselSessionId ipc_id) {
    return m_session_status_map[ipc_id];
  }
  SessionStatus& new_session_status(WeaselSessionId ipc_id) {
    return m_session_status_map[ipc_id] = SessionStatus();
  }

  AppOptionsByAppName m_app_options;
  weasel::UI* m_ui;  // reference
  DWORD m_active_session;
  bool m_disabled;
  std::string m_last_schema_id;
  std::string m_last_app_name;
  weasel::UIStyle m_base_style;
  std::map<std::string, bool> m_show_notifications;
  std::map<std::string, bool> m_show_notifications_base;
  std::function<void()> _UpdateUICallback;

  static void OnNotify(void* context_object,
                       uintptr_t session_id,
                       const char* message_type,
                       const char* message_value);
  static std::string m_message_type;
  static std::string m_message_value;
  static std::string m_message_label;
  static std::string m_option_name;
  static std::mutex m_notifier_mutex;
  SessionStatusMap m_session_status_map;
  bool m_current_dark_mode;
  bool m_global_ascii_mode;
  int m_show_notifications_time;
  DWORD m_pid;

  weasel::P2cEngine m_p2c;
  std::map<WeaselSessionId, P2cSession> m_p2c_sessions;
  std::mutex m_p2c_mutex;
  std::function<void()> m_ui_refresh;
  bool m_p2c_enabled = false;
};
