// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 */

#include <CppUTestExt/MockSupport.h>
#include "mock_sp_messaging.h"

void expect_sp_msg_wait(const struct sp_msg *msg, sp_result result)
{
	mock()
		.expectOneCall("sp_msg_wait")
		.withOutputParameterReturning("msg", msg, sizeof(*msg))
		.andReturnValue(result);
}

sp_result sp_msg_wait(struct sp_msg *msg)
{
	return mock()
		.actualCall("sp_msg_wait")
		.withOutputParameter("msg", msg)
		.returnIntValue();
}

void expect_sp_yield(sp_result result)
{
	mock()
		.expectOneCall("sp_yield")
		.andReturnValue(result);
}

sp_result sp_yield(void)
{
	return mock()
		.actualCall("sp_yield")
		.returnIntValue();
}

void expect_sp_msg_send_direct_req(const struct sp_msg *req,
				   const struct sp_msg *resp,
				   sp_result result)
{
	mock()
		.expectOneCall("sp_msg_send_direct_req")
		.withMemoryBufferParameter("req", (const unsigned char *)req, sizeof(*req))
		.withOutputParameterReturning("resp", resp, sizeof(*resp))
		.andReturnValue(result);
}

sp_result sp_msg_send_direct_req(const struct sp_msg *req, struct sp_msg *resp)
{
	return mock()
		.actualCall("sp_msg_send_direct_req")
		.withMemoryBufferParameter("req", (const unsigned char *)req, sizeof(*req))
		.withOutputParameter("resp", resp)
		.returnIntValue();
}

void expect_sp_msg_send_direct_resp(const struct sp_msg *resp,
				    const struct sp_msg *req,
				    sp_result result)
{
	mock()
		.expectOneCall("sp_msg_send_direct_resp")
		.withMemoryBufferParameter("resp", (const unsigned char *)resp, sizeof(*resp))
		.withOutputParameterReturning("req", req, sizeof(*req))
		.andReturnValue(result);
}

sp_result sp_msg_send_direct_resp(const struct sp_msg *resp,
				  struct sp_msg *req)
{
	return mock()
		.actualCall("sp_msg_send_direct_resp")
		.withMemoryBufferParameter("resp", (const unsigned char *)resp, sizeof(*resp))
		.withOutputParameter("req", req)
		.returnIntValue();
}

#if FFA_DIRECT_MSG_ROUTING_EXTENSION
void expect_sp_msg_send_rc_req(const struct sp_msg *req,
			       const struct sp_msg *resp,
			       sp_result result)
{
	mock()
		.expectOneCall("sp_msg_send_rc_req")
		.withMemoryBufferParameter("req", (const unsigned char *)req, sizeof(*req))
		.withOutputParameterReturning("resp", resp, sizeof(*resp))
		.andReturnValue(result);
}

sp_result sp_msg_send_rc_req(const struct sp_msg *req, struct sp_msg *resp)
{
	return mock()
		.actualCall("sp_msg_send_rc_req")
		.withMemoryBufferParameter("req", (const unsigned char *)req, sizeof(*req))
		.withOutputParameter("resp", resp)
		.returnIntValue();
}
#endif /* FFA_DIRECT_MSG_ROUTING_EXTENSION */
