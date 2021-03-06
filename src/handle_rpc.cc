// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: handle_rpc.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-13 17:11:20



#include "src/handle_rpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "src/service_register.h"
#include "src/saf_const.h"
#include "src/rpc_util.h"

using namespace google::protobuf;  // NOLINT

namespace sails {

void HandleRPC::do_handle(sails::RequestPacket *request,
                          sails::ResponsePacket *response,
                          base::HandleChain<sails::RequestPacket *,
                          sails::ResponsePacket *> *chain) {
  if (request != NULL) {
    if (request->type() == MessageType::PING) {
      response->set_version(request->version());
      response->set_type(request->type());
      response->set_sn(request->sn());
      response->set_ret(ErrorCode::ERR_SUCCESS);
      return;
    }
    response->set_ret(ErrorCode::ERR_OTHER);
    // cout << "service_name :" << request->servicename()
    // <<  "fun:" << request->funcname() << endl;
    if (!request->servicename().empty() && !request->funcname().empty()) {
      google::protobuf::Service* service
          = ServiceRegister::instance()->get_service(request->servicename());
      if (service != NULL) {
        // or find by method_index
        const MethodDescriptor *method_desc
            = service->GetDescriptor()->FindMethodByName(request->funcname());
        if (method_desc != NULL) {
          Message *request_msg
              = service->GetRequestPrototype(method_desc).New();
          Message *response_mg
              = service->GetResponsePrototype(method_desc).New();

          bool parse_success = false;
          if (request->type() == MessageType::RPC_REQUEST) {
            response->set_type(MessageType::RPC_RESPONSE);
            parse_success = request_msg->ParseFromString(request->detail());
          } else if (request->type() == MessageType::RPC_REQUEST_JSON) {
            response->set_type(MessageType::RPC_RESPONSE_JSON);
            sails::JsonPBConvert convert;
            parse_success = convert.FromJson(request->detail(), request_msg);
          }

          if (parse_success) {
            service->CallMethod(
                method_desc, NULL, request_msg, response_mg, NULL);

            std::string response_body = "";
            if (response->type() == MessageType::RPC_RESPONSE) {
              response_body = response_mg->SerializeAsString();
            } else if (response->type() == MessageType::RPC_RESPONSE_JSON) {
              sails::JsonPBConvert convert;
              response_body = convert.ToJson(*response_mg);
            }
            response->set_detail(response_body);
            response->set_version(request->version());
            response->set_sn(request->sn());
            response->set_ret(ErrorCode::ERR_SUCCESS);
          } else {
            response->set_ret(ErrorCode::ERR_PARAM);
          }
          delete request_msg;
          delete response_mg;
        } else {
          response->set_ret(ErrorCode::ERR_FUN_NAME);
        }
      } else {
        response->set_ret(ErrorCode::ERR_SERVICE_NAME);
      }
    } else {
      response->set_ret(ErrorCode::ERR_SERVICE_NAME);
    }
  }
  chain->do_handle(request, response);
}

HandleRPC::~HandleRPC() {
  //    printf("delete handle_rpc\n");
}

}  // namespace sails



