/**
  * Copyright (c) <2016> granada <afernandez@cookinapps.io>
  *
  * This source code is licensed under the MIT license.
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  * SOFTWARE.
  *
  */
#include "granada/http/session/session.h"

namespace granada{
  namespace http{
    namespace session{

      std::vector<std::string> Session::DEFAULT_SESSIONS_TOKEN_SUPPORT = {entity_keys::session_token_support,default_strings::session_second_token_support};

      void Session::LoadProperties(){
        token_label_.assign(session_handler()->GetProperty(entity_keys::session_token_label));
        if (token_label_.empty()){
          token_label_.assign(default_strings::session_token_label);
        }

        session_token_support_ = session_handler()->GetProperty(entity_keys::session_token_support);

        std::string session_timeout_str = session_handler()->GetProperty(entity_keys::session_timeout);
        if (session_timeout_str.empty()){
          session_timeout_ = default_numbers::session_timeout;
        }else{
          try{
            session_timeout_ = std::stol(session_timeout_str);
          }catch(const std::logic_error& e){
            session_timeout_ = default_numbers::session_timeout;
          }
        }
      }


      const bool Session::LoadSession(web::http::http_request &request,web::http::http_response &response){
        if (session_token_support_.empty()){
          // request token by default
          session_token_support_ = Session::DEFAULT_SESSIONS_TOKEN_SUPPORT[0];
        }

        // search and retrieve token from cookies.
        if (session_token_support_ == entity_keys::session_cookie){
          bool session_exists = false;
          std::unordered_map<std::string, std::string> cookies = granada::http::parser::ParseCookies(request);
          auto it = cookies.find(token_label_);
          if (it != cookies.end()){
            std::string token = it->second;
            session_exists = LoadSession(token);
          }
          if(!session_exists){
            Open(response);
          }
          return true;
        }
        return LoadSession(request);
      }


      const bool Session::LoadSession(web::http::http_request &request){
        if (session_token_support_.empty()){
          // request token by default
          session_token_support_ = Session::DEFAULT_SESSIONS_TOKEN_SUPPORT[1];
        }

        // retrieve token from body json.
        if (session_token_support_ == entity_keys::session_json){
          try{
            const web::json::value& obj = request.extract_json().get();
            if (obj.is_object() && obj.has_field(token_label_)){
              const web::json::value token_json = obj.at(token_label_);
              if (token_json.is_string()){
                std::string token = token_json.as_string();
                return LoadSession(token);
              }
            }
          }catch(const std::exception& e){}
        }else{
          // retrieve token from query string.
          if (session_token_support_ == entity_keys::session_query){
            std::string query = request.relative_uri().query();
            std::vector<std::string> keys_and_values;
            granada::util::string::split(query,'&',keys_and_values);
            int len = keys_and_values.size();
            while (len--) {
              std::vector<std::string> key_and_value;
              granada::util::string::split(keys_and_values[len],'=',key_and_value);
              if (key_and_value.size() > 1 && key_and_value[0] == token_label_){
                std::string token = key_and_value[1];
                return LoadSession(token);
              }
            }
          }
        }
        return false;
      }


      const bool Session::LoadSession(const std::string& token){
        if (!token.empty()){
          // use session handler to load session from wherever the sessions are stored.
          // If session is found the value of this session will be replaced by the
          // found session.
          session_handler()->LoadSession(token,this);
          if (!token_.empty()){
            // session found, update the session. For example the session update time,
            // so session is kept alive.
            Update();
            return true;
          }
        }
        return false;
      }

      void Session::Open(){
        // if token already exist, delete the copy of the "old" session
        // from where it is stored, so its not used again.
        Close();

        // generate token
        token_.assign(session_handler()->GenerateToken());
        // check if session do not already exist.
        if (session_handler()->SessionExists(token_)){
          // a session with this token already exist!
          // repeat the process of opening a new session.
          Open();
        }else{
          // session is created, update it, for example the sesison update time.
          Update();
        }
      }

      void Session::Open(web::http::http_response &response){
        // open session
        Open();
        if (session_token_support_ == entity_keys::session_cookie){
          // add cookie with token
          response.headers().add(entity_keys::session_set_cookie, token_label_ + "=" + token_ + "; path=/");
        }
      }

      void Session::Update(){
        // set the update time to now.
        update_time_ = std::time(nullptr);
        // save the session wherever all the sessions are stored.
        //session_handler()->SaveSession(this);
      }

      void Session::Close(){
        // removes a session from wherever sessions are stored.
        web::json::value session_json = to_json();
        close_callbacks()->CallAll(session_json);
        roles()->RemoveAll();
        session_handler()->DeleteSession(this);
      }

      const bool Session::IsValid(){
        return !IsTimedOut();
      }

      const bool Session::IsGarbage(){
        return !IsValid();
      }


      const bool Session::IsTimedOut(){
        return IsTimedOut(0);
      }

      const bool Session::IsTimedOut(const long int& extra_seconds){
        if ( session_timeout_ < 0){
          return false;
        }else{
          return granada::util::time::is_timedout(update_time_,session_timeout_,extra_seconds);
        }
      }
    }
  }
}
