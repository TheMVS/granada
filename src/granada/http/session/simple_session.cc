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
#include "granada/http/session/simple_session.h"

namespace granada{
  namespace http{
    namespace session{

      // we use a session handler that use a map shared by all user to store the sessions.
      std::unique_ptr<granada::http::session::SharedMapSessionHandler> SimpleSession::session_handler_(new granada::http::session::SharedMapSessionHandler());
      long SimpleSession::DEFAULT_SESSION_CLEAN_EXTRA_TIMEOUT = 0;

      SimpleSession::SimpleSession(web::http::http_request &request,web::http::http_response &response){
        roles_ = std::shared_ptr<granada::http::session::Roles>(new granada::http::session::MapRoles(this));
        LoadProperties();
        Session::LoadSession(request,response);
      }

      SimpleSession::SimpleSession(web::http::http_request &request){
        roles_ = std::shared_ptr<granada::http::session::Roles>(new granada::http::session::MapRoles(this));
        LoadProperties();
        Session::LoadSession(request);
      }

      SimpleSession::SimpleSession(const std::string& token){
        roles_ = std::shared_ptr<granada::http::session::Roles>(new granada::http::session::MapRoles(this));
        LoadProperties();
        Session::LoadSession(token);
      }


      void SimpleSession::LoadProperties(){
        Session::LoadProperties();
        std::string session_clean_extra_timeout_str(session_handler()->GetProperty("session_clean_extra_timeout"));
        if (session_clean_extra_timeout_str.empty()){
          session_clean_extra_timeout_ = SimpleSession::DEFAULT_SESSION_CLEAN_EXTRA_TIMEOUT;
        }else{
          try{
            session_clean_extra_timeout_ = std::stol(session_clean_extra_timeout_str);
          }catch(const std::exception& e){
            session_clean_extra_timeout_ = SimpleSession::DEFAULT_SESSION_CLEAN_EXTRA_TIMEOUT;
          }
        }
      }

      void SimpleSession::Open(){
        Session::Open();
      }

      void SimpleSession::Update(){
        update_time_ = std::time(nullptr);
        granada::http::session::SimpleSession* simple_session_ptr = new granada::http::session::SimpleSession();
        *simple_session_ptr = *this;
        std::shared_ptr<granada::http::session::SimpleSession> simple_session_shared_ptr(simple_session_ptr);
        // save the session wherever all the sessions are stored.
        session_handler()->SaveSession(simple_session_shared_ptr);
      }

      void SimpleSession::Close(){
        Session::Close();
      }

      const bool SimpleSession::IsValid(){
        return Session::IsValid();
      }

      const bool SimpleSession::IsGarbage(){
        return Session::IsGarbage();
      }
    }
  }
}