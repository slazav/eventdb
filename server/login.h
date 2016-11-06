#ifndef LOGIN_H
#define LOGIN_H

#include "cfg.h"
#include "err.h"
#include "jsonxx/jsonxx.h"

/* json_t *get_login_info(const CFG & cfg, const char *tok);
 *
 * Build user login information in JSON format using secret token.
 * There are two authentification mechanisms:
 * - test_users object in the config file can contain
 *   token:login_info pair (used only for tests).
 * - login information is asked from loginza.ru service.
 *   loginza secret and id should be in config file.
 *
 * Returned Json object  contains following fields:
 * - id   -- user identity in URL form
 * - site -- short provider name: lj, fb, google, etc.
 * - name -- user name
 *
 * Login information is also saved in the log file.
 */

Json get_login_info(const CFG & cfg, const char *token);

#endif
