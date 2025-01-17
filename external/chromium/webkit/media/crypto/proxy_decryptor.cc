// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/media/crypto/proxy_decryptor.h"

// WebMediaPlayer.h needs uint8_t defined
// See: https://bugs.webkit.org/show_bug.cgi?id=92031
#include <stdint.h>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#if defined(_DEBUG)
#include "base/string_number_conversions.h"
#endif
#include "media/base/audio_decoder_config.h"
#if !defined(__LB_SHELL__)
#include "media/base/decoder_buffer.h"
#endif
#include "media/base/decryptor_client.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#if !defined(__LB_SHELL__)
#include "media/crypto/aes_decryptor.h"
#else
#include "media/base/shell_buffer_factory.h"
#include "media/crypto/shell_decryptor_factory.h"
#endif
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebMediaPlayerClient.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebString.h"
#include "webkit/media/crypto/key_systems.h"
#if !defined(__LB_SHELL__)
#include "webkit/media/crypto/ppapi_decryptor.h"
#include "webkit/plugins/ppapi/ppapi_plugin_instance.h"
#include "webkit/plugins/ppapi/ppapi_webplugin_impl.h"
#endif

namespace webkit_media {

#if !defined(__LB_SHELL__)
static scoped_refptr<webkit::ppapi::PluginInstance> CreatePluginInstance(
    const std::string& plugin_type,
    WebKit::WebMediaPlayerClient* web_media_player_client,
    WebKit::WebFrame* web_frame) {
  DCHECK(web_media_player_client);
  DCHECK(web_frame);

  WebKit::WebPlugin* web_plugin = web_media_player_client->createHelperPlugin(
      WebKit::WebString::fromUTF8(plugin_type), web_frame);
  if (!web_plugin)
    return NULL;

  DCHECK(!web_plugin->isPlaceholder());  // Prevented by WebKit.
  // Only Pepper plugins are supported, so it must be a ppapi object.
  webkit::ppapi::WebPluginImpl* ppapi_plugin =
      static_cast<webkit::ppapi::WebPluginImpl*>(web_plugin);
  return ppapi_plugin->instance();
}
#endif

ProxyDecryptor::ProxyDecryptor(
    media::DecryptorClient* decryptor_client,
    WebKit::WebMediaPlayerClient* web_media_player_client,
    WebKit::WebFrame* web_frame)
    : client_(decryptor_client),
      web_media_player_client_(web_media_player_client),
      web_frame_(web_frame) {
}

ProxyDecryptor::~ProxyDecryptor() {
}

// TODO(xhwang): Support multiple decryptor notification request (e.g. from
// video and audio decoders). The current implementation is okay for the current
// media pipeline since we initialize audio and video decoders in sequence.
// But ProxyDecryptor should not depend on media pipeline's implementation
// detail.
void ProxyDecryptor::SetDecryptorReadyCB(
     const media::DecryptorReadyCB& decryptor_ready_cb) {
  base::AutoLock auto_lock(lock_);

  // Cancels the previous decryptor request.
  if (decryptor_ready_cb.is_null()) {
    if (!decryptor_ready_cb_.is_null())
      base::ResetAndReturn(&decryptor_ready_cb_).Run(NULL);
    return;
  }

  // Normal decryptor request.
  DCHECK(decryptor_ready_cb_.is_null());
  if (decryptor_) {
    decryptor_ready_cb.Run(decryptor_.get());
    return;
  }
  decryptor_ready_cb_ = decryptor_ready_cb;
}

bool ProxyDecryptor::GenerateKeyRequest(const std::string& key_system,
                                        const std::string& type,
                                        const uint8* init_data,
                                        int init_data_length) {
  // We do not support run-time switching of decryptors. GenerateKeyRequest()
  // only creates a new decryptor when |decryptor_| is not initialized.
  DVLOG(1) << "GenerateKeyRequest: key_system = " << key_system;

  base::AutoLock auto_lock(lock_);

  if (!decryptor_) {
    decryptor_ = CreateDecryptor(key_system);
    if (!decryptor_) {
      client_->KeyError(key_system, "", media::Decryptor::kUnknownError, 0);
      return false;
    }
  }

  if (!decryptor_->GenerateKeyRequest(key_system, type,
                                      init_data, init_data_length)) {
    decryptor_.reset();
    return false;
  }

  if (!decryptor_ready_cb_.is_null())
    base::ResetAndReturn(&decryptor_ready_cb_).Run(decryptor_.get());

  return true;
}

void ProxyDecryptor::AddKey(const std::string& key_system,
                            const uint8* key,
                            int key_length,
                            const uint8* init_data,
                            int init_data_length,
                            const std::string& session_id) {
  DVLOG(1) << "AddKey()";
#if defined(_DEBUG)
  std::string hex = base::HexEncode(key, key_length);
  DLOG(INFO) << "DRM Key Response: " << hex;
#endif

  // WebMediaPlayerImpl ensures GenerateKeyRequest() has been called.
  decryptor_->AddKey(key_system, key, key_length, init_data, init_data_length,
                     session_id);
}

void ProxyDecryptor::CancelKeyRequest(const std::string& key_system,
                                      const std::string& session_id) {
  DVLOG(1) << "CancelKeyRequest()";

  // WebMediaPlayerImpl ensures GenerateKeyRequest() has been called.
  decryptor_->CancelKeyRequest(key_system, session_id);
}

void ProxyDecryptor::RegisterKeyAddedCB(StreamType stream_type,
                                        const KeyAddedCB& key_added_cb) {
  NOTREACHED() << "KeyAddedCB should not be registered with ProxyDecryptor.";
}

#if defined(__LB_SHELL__)
void ProxyDecryptor::Decrypt(
    StreamType stream_type,
    const scoped_refptr<media::ShellBuffer>& encrypted,
    const DecryptCB& decrypt_cb) {
  NOTREACHED() << "ProxyDecryptor does not support decryption";
}
#else
void ProxyDecryptor::Decrypt(
    StreamType stream_type,
    const scoped_refptr<media::DecoderBuffer>& encrypted,
    const DecryptCB& decrypt_cb) {
  NOTREACHED() << "ProxyDecryptor does not support decryption";
}
#endif

void ProxyDecryptor::CancelDecrypt(StreamType stream_type) {
  base::AutoLock auto_lock(lock_);

  if (decryptor_)
    decryptor_->CancelDecrypt(stream_type);
}

void ProxyDecryptor::InitializeAudioDecoder(
    scoped_ptr<media::AudioDecoderConfig> config,
    const DecoderInitCB& init_cb) {
  NOTREACHED() << "ProxyDecryptor does not support audio decoding";
}

void ProxyDecryptor::InitializeVideoDecoder(
    scoped_ptr<media::VideoDecoderConfig> config,
    const DecoderInitCB& init_cb) {
  NOTREACHED() << "ProxyDecryptor does not support video decoding";
}

#if defined(__LB_SHELL__)
void ProxyDecryptor::DecryptAndDecodeAudio(
    const scoped_refptr<media::ShellBuffer>& encrypted,
    const AudioDecodeCB& audio_decode_cb) {
  NOTREACHED() << "ProxyDecryptor does not support audio decoding";
}

void ProxyDecryptor::DecryptAndDecodeVideo(
    const scoped_refptr<media::ShellBuffer>& encrypted,
    const VideoDecodeCB& video_decode_cb) {
  NOTREACHED() << "ProxyDecryptor does not support video decoding";
}
#else
void ProxyDecryptor::DecryptAndDecodeAudio(
    const scoped_refptr<media::DecoderBuffer>& encrypted,
    const AudioDecodeCB& audio_decode_cb) {
  NOTREACHED() << "ProxyDecryptor does not support audio decoding";
}

void ProxyDecryptor::DecryptAndDecodeVideo(
    const scoped_refptr<media::DecoderBuffer>& encrypted,
    const VideoDecodeCB& video_decode_cb) {
  NOTREACHED() << "ProxyDecryptor does not support video decoding";
}
#endif

void ProxyDecryptor::ResetDecoder(StreamType stream_type) {
  NOTREACHED() << "ProxyDecryptor does not support audio/video decoding";
}

void ProxyDecryptor::DeinitializeDecoder(StreamType stream_type) {
  NOTREACHED() << "ProxyDecryptor does not support audio/video decoding";
}

#if !defined(__LB_SHELL__)
scoped_ptr<media::Decryptor> ProxyDecryptor::CreatePpapiDecryptor(
    const std::string& key_system) {
  DCHECK(client_);
  DCHECK(web_media_player_client_);
  DCHECK(web_frame_);

  std::string plugin_type = GetPluginType(key_system);
  DCHECK(!plugin_type.empty());
  const scoped_refptr<webkit::ppapi::PluginInstance>& plugin_instance =
    CreatePluginInstance(plugin_type, web_media_player_client_, web_frame_);
  if (!plugin_instance) {
    DVLOG(1) << "ProxyDecryptor: plugin instance creation failed.";
    return scoped_ptr<media::Decryptor>();
  }

  return scoped_ptr<media::Decryptor>(new PpapiDecryptor(client_,
                                                         plugin_instance));
}
#endif

scoped_ptr<media::Decryptor> ProxyDecryptor::CreateDecryptor(
    const std::string& key_system) {
  DCHECK(client_);

#if !defined(__LB_SHELL__)
  if (CanUseAesDecryptor(key_system))
    return scoped_ptr<media::Decryptor>(new media::AesDecryptor(client_));

  // We only support AesDecryptor and PpapiDecryptor. So if we cannot
  // use the AesDecryptor, then we'll try to create a PpapiDecryptor for given
  // |key_system|.
  return CreatePpapiDecryptor(key_system);
#else
  // lb_shell doesn't support ppapi or AesDecryptor, so we have our own
  // decryptor factory to handle cdm support.
  return scoped_ptr<media::Decryptor>(
      media::ShellDecryptorFactory::Create(key_system, client_));
#endif
}

}  // namespace webkit_media
