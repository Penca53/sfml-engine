#pragma once

#include <string>

namespace ng {

/// @brief Represents a state in a finite state machine (FSM).
/// @tparam TContext The type of the context object that the state machine operates on.
template <typename TContext>
class State {
  // FSM needs to be able to call OnEnter, Update, OnExit and SetContext.
  template <typename>
  friend class FSM;

 public:
  /// @brief Type alias for the state identifier.
  using ID = std::string;

  /// @brief Constructs a State with a unique ID.
  /// @param id The identifier for this state.
  explicit State(ID id) : id_(std::move(id)) {}

  virtual ~State() = default;

  State(const State& other) = default;
  State& operator=(const State& other) = default;
  State(State&& other) = default;
  State& operator=(State&& other) = default;

  /// @brief Returns the unique identifier of the state.
  /// @return The ID of this state.
  [[nodiscard]] const ID& GetID() const { return id_; }

 protected:
  /// @brief Returns a pointer to the context object.
  /// @return A pointer to the context. Can be null if the state is not associated with an FSM yet.
  TContext* GetContext() { return context_; }

  /// @brief Called when the state is entered.
  virtual void OnEnter() {}

  /// @brief Called during the update phase while this state is active.
  virtual void Update() {}

  /// @brief Called when the state is exited.
  virtual void OnExit() {}

 private:
  /// @brief Sets the context object for this state. Called by the FSM.
  /// @param context A pointer to the context object. This pointer must not be null.
  void SetContext(TContext* context) { context_ = context; }

  // The unique identifier of the state.
  ID id_;
  // Pointer to the context object that the FSM operates on.
  // Can be null if the state is not associated with an FSM yet.
  TContext* context_ = nullptr;
};

}  // namespace ng