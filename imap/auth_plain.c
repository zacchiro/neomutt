/**
 * @file
 * IMAP plain authentication method
 *
 * @authors
 * Copyright (C) 1999-2001,2005,2009 Brendan Cully <brendan@kublai.com>
 * Copyright (C) 2016 Pietro Cerutti <gahr@gahr.ch>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page imap_auth_plain IMAP plain authentication method
 *
 * IMAP plain authentication method
 */

#include "config.h"
#include "imap_private.h"
#include "mutt/mutt.h"
#include "conn/conn.h"
#include "auth.h"
#include "mutt_account.h"
#include "mutt_logging.h"
#include "mutt_socket.h"

/**
 * imap_auth_plain - SASL PLAIN support
 * @param mdata Imap Mailbox data
 * @param method Name of this authentication method
 * @retval enum Result, e.g. #IMAP_AUTH_SUCCESS
 */
enum ImapAuthRes imap_auth_plain(struct ImapMboxData *mdata, const char *method)
{
  int rc = IMAP_CMD_CONTINUE;
  enum ImapAuthRes res = IMAP_AUTH_SUCCESS;
  static const char auth_plain_cmd[] = "AUTHENTICATE PLAIN";
  char buf[STRING] = { 0 };

  if (mutt_account_getuser(&mdata->conn->account) < 0)
    return IMAP_AUTH_FAILURE;
  if (mutt_account_getpass(&mdata->conn->account) < 0)
    return IMAP_AUTH_FAILURE;

  mutt_message(_("Logging in..."));

  /* Prepare full AUTHENTICATE PLAIN message */
  mutt_sasl_plain_msg(buf, sizeof(buf), auth_plain_cmd, mdata->conn->account.user,
                      mdata->conn->account.user, mdata->conn->account.pass);

  if (mutt_bit_isset(mdata->capabilities, SASL_IR))
  {
    imap_cmd_start(mdata, buf);
  }
  else
  {
    /* Split the message so we send AUTHENTICATE PLAIN first, and the
     * credentials after the first command continuation request */
    buf[sizeof(auth_plain_cmd) - 1] = '\0';
    imap_cmd_start(mdata, buf);
    while (rc == IMAP_CMD_CONTINUE)
    {
      rc = imap_cmd_step(mdata);
    }
    if (rc == IMAP_CMD_RESPOND)
    {
      mutt_str_strcat(buf + sizeof(auth_plain_cmd),
                      sizeof(buf) - sizeof(auth_plain_cmd), "\r\n");
      mutt_socket_send(mdata->conn, buf + sizeof(auth_plain_cmd));
    }
  }

  while (rc == IMAP_CMD_CONTINUE)
  {
    rc = imap_cmd_step(mdata);
  }

  if (rc == IMAP_CMD_BAD)
  {
    res = IMAP_AUTH_UNAVAIL;
  }
  else if (rc == IMAP_CMD_NO)
  {
    mutt_error(_("Login failed"));
    res = IMAP_AUTH_FAILURE;
  }

  mutt_clear_error(); /* clear "Logging in...".  fixes #3524 */
  return res;
}
