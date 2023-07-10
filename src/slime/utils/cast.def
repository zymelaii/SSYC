#pragma once

#include <assert.h>

#define RegisterAsCastWithoutSuffix(typeId, name, suffix, enumScope) \
 name##suffix *as##name() {                                          \
  assert(typeId == enumScope::name);                                 \
  return reinterpret_cast<name##suffix *>(this);                     \
 }                                                                   \
 const name##suffix *as##name() const {                              \
  assert(typeId == enumScope::name);                                 \
  return reinterpret_cast<const name##suffix *>(this);               \
 }

#define RegisterTryIntoCastWithoutSuffix(typeId, name, suffix, enumScope) \
 name##suffix *tryInto##name() {                                          \
  return typeId == enumScope::name ? as##name() : nullptr;                \
 }                                                                        \
 const name##suffix *tryInto##name() const {                              \
  return typeId == enumScope::name ? as##name() : nullptr;                \
 }

#define RegisterCastWithoutSuffix(typeId, name, suffix, enumScope) \
 RegisterAsCastWithoutSuffix(typeId, name, suffix, enumScope)      \
     RegisterTryIntoCastWithoutSuffix(typeId, name, suffix, enumScope)

#define RegisterAsCast(typeId, name, suffix, enumScope) \
 name##suffix *as##name##suffix() {                     \
  assert(typeId == enumScope::name);                    \
  return reinterpret_cast<name##suffix *>(this);        \
 }                                                      \
 const name##suffix *as##name##suffix() const {         \
  assert(typeId == enumScope::name);                    \
  return reinterpret_cast<const name##suffix *>(this);  \
 }

#define RegisterTryIntoCast(typeId, name, suffix, enumScope)       \
 name##suffix *tryInto##name##suffix() {                           \
  return typeId == enumScope::name ? as##name##suffix() : nullptr; \
 }                                                                 \
 const name##suffix *tryInto##name##suffix() const {               \
  return typeId == enumScope::name ? as##name##suffix() : nullptr; \
 }

#define RegisterCast(typeId, name, suffix, enumScope) \
 RegisterAsCast(typeId, name, suffix, enumScope)      \
     RegisterTryIntoCast(typeId, name, suffix, enumScope)

#define RegisterAsCastWithoutSuffixDecl(typeId, name, suffix, enumScope) \
 inline name##suffix       *as##name();                                  \
 inline const name##suffix *as##name() const;

#define RegisterTryIntoCastWithoutSuffixDecl(typeId, name, suffix, enumScope) \
 inline name##suffix       *tryInto##name();                                  \
 inline const name##suffix *tryInto##name() const;

#define RegisterCastWithoutSuffixDecl(typeId, name, suffix, enumScope) \
 RegisterAsCastWithoutSuffixDecl(typeId, name, suffix, enumScope)      \
     RegisterTryIntoCastWithoutSuffixDecl(typeId, name, suffix, enumScope)

#define RegisterAsCastDecl(typeId, name, suffix, enumScope) \
 inline name##suffix       *as##name##suffix();             \
 inline const name##suffix *as##name##suffix() const;

#define RegisterTryIntoCastDecl(typeId, name, suffix, enumScope) \
 inline name##suffix       *tryInto##name##suffix();             \
 inline const name##suffix *tryInto##name##suffix() const;

#define RegisterCastDecl(typeId, name, suffix, enumScope) \
 RegisterAsCastDecl(typeId, name, suffix, enumScope)      \
     RegisterTryIntoCastDecl(typeId, name, suffix, enumScope)

#define RegisterAsCastWithoutSuffixImpl(typeId, name, suffix, enumScope) \
 inline name##suffix *suffix::as##name() {                               \
  assert(typeId == enumScope::name);                                     \
  return static_cast<name##suffix *>(this);                              \
 }                                                                       \
 const inline name##suffix *suffix::as##name() const {                   \
  assert(typeId == enumScope::name);                                     \
  return static_cast<const name##suffix *>(this);                        \
 }

#define RegisterTryIntoCastWithoutSuffixImpl(typeId, name, suffix, enumScope) \
 inline name##suffix *suffix::tryInto##name() {                               \
  return typeId == enumScope::name ? as##name() : nullptr;                    \
 }                                                                            \
 inline const name##suffix *suffix::tryInto##name() const {                   \
  return typeId == enumScope::name ? as##name() : nullptr;                    \
 }

#define RegisterCastWithoutSuffixImpl(typeId, name, suffix, enumScope) \
 RegisterAsCastWithoutSuffixImpl(typeId, name, suffix, enumScope)      \
     RegisterTryIntoCastWithoutSuffixImpl(typeId, name, suffix, enumScope)

#define RegisterAsCastImpl(typeId, name, suffix, enumScope)    \
 inline name##suffix *suffix::as##name##suffix() {             \
  assert(typeId == enumScope::name);                           \
  return static_cast<name##suffix *>(this);                    \
 }                                                             \
 inline const name##suffix *suffix::as##name##suffix() const { \
  assert(typeId == enumScope::name);                           \
  return static_cast<const name##suffix *>(this);              \
 }

#define RegisterTryIntoCastImpl(typeId, name, suffix, enumScope)    \
 inline name##suffix *suffix::tryInto##name##suffix() {             \
  return typeId == enumScope::name ? as##name##suffix() : nullptr;  \
 }                                                                  \
 inline const name##suffix *suffix::tryInto##name##suffix() const { \
  return typeId == enumScope::name ? as##name##suffix() : nullptr;  \
 }

#define RegisterCastImpl(typeId, name, suffix, enumScope) \
 RegisterAsCastImpl(typeId, name, suffix, enumScope)      \
     RegisterTryIntoCastImpl(typeId, name, suffix, enumScope)
