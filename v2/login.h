#ifndef LOGIN_H
#define LOGIN_H

#include "cfg.h"
#include <jansson.h>

/* json_t *get_login_info(const CFG & cfg, const char *tok);
 *
 * Build user login information in JSON format using secret token.
 * There are two authentification mechanisms:
 * - test_users object in the config file can contain
 *   token:login_info pair (used only for tests).
 * - login information is asked from loginza.ru service.
 *   loginza secret and id should be in config file.
 *
 * Returned json_t object  contains following fields:
 * - identity (in URL form)
 * - provider (short name: lj, fb, google, etc.)
 * - alias -- default alias (used only when user is created)
 * - full_name -- user name
 * The object should be freed after use.
 *
 * Login information is also saved in the log file ($logsdir/login.log).
 */

json_t *
get_login_info(const CFG & cfg, const char *token);

#endif
