/* Simple C program that connects to MySQL Database server*/
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define INSERT_DATA "INSERT INTO iaq_sensordata VALUES" \
                    "(DEFAULT, '%s', %.1f, %.1f, %.0f, %.0f, %.0f, %.2f, '%s')"

#define UPDATE_NODE "UPDATE iaq_sensornode SET temp=%.1f, humi=%.1f, " \
                    "iaqengine=%.0f, tgs2600=%.0f, tgs2602=%.0f, " \
                    "formaldehyde=%.2f, time='%s' " \
                    "WHERE node_id='%s'"

int main() {
   MYSQL *conn;
   MYSQL_RES *res;
   MYSQL_ROW row;
   char *server = "localhost";
   char *user = "root";
   char *password = "joe159357"; /* set me first */
   char *database = "iaq";
   conn = mysql_init(NULL);
   /* Connect to database */
   if (!mysql_real_connect(conn, server,
         user, password, database, 0, NULL, 0)) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      exit(1);
   }

    char node_id[] = "1001";
    float data[6] = {29.5, 41.4, 1567, 979, 734, 1.23}; 
    char query[256];
    char now[32];
    time_t tt = time(NULL);
    strftime(now, 32, "%F %T", gmtime(&tt));

    /* INSERT */
    snprintf(query, 256, INSERT_DATA, node_id, data[0], data[1], 
            data[2], data[3], data[4], data[5], now);
    printf("%s\n", query);
    if(mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    /* UPDATE */
    snprintf(query, 256, UPDATE_NODE, data[0], data[1], data[2], data[3], 
            data[4], data[5], now, node_id);
    printf("%s\n", query);
    if(mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

   /* close connection */
   mysql_close(conn);
   return 0;
}
