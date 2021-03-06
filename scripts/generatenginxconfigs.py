#!/usr/bin/python2
cfg_text = '''#user  nobody;
worker_processes  1;
#error_log  logs/error.log;
#error_log  logs/error.log  notice;
#error_log  logs/error.log  info;

#pid        logs/nginx.pid;
daemon off;
master_process off;

events {
    worker_connections  1024;
}


# By default nginx retardedly clears all the environment variables.... here we hack to preserve the ones we need.
env TZ;
env FLAGS_v;
env FLAGS_flow_scheduling_cost_model;
env FLAGS_debug_flow_graph;
env FLAGS_spawn_interval;
env FLAGS_num_tasks;
env FLAGS_job_arrival_lambda;
env FLAGS_job_size_lambda;
env FLAGS_job_arrival_scaling_factor;
env FLAGS_task_duration_scaling_factor;
env FLAGS_output;
env FLAGS_peer_adjacent;
env FLAGS_simulation_runtime;
env FLAGS_use_prefs;
env FLAGS_schedule_use_nested;
env FLAGS_schedule_use_peered;
env FLAGS_non_preferred_penalty_factor;
env FLAGS_listen_uri;
env FLAGS_NO_RENEGOTIATE_CIPHERS;
env FLAGS_coordinator_uri;
env FLAGS_resource_id;
env FLAGS_task_id;
env FLAGS_heartbeat_interval;
env FLAGS_tasklib_application;
env FLAGS_set;
env FLAGS_value;
env FLAGS_platform;
env FLAGS_debug_tasks;
env FLAGS_debug_interactively;
env FLAGS_perf_monitoring;
env FLAGS_http_ui;
env FLAGS_scheduler;
env FLAGS_include_local_resources;
env FLAGS_parent_uri;
env FLAGS_http_ui_port;
env FLAGS_name;
env FLAGS_nginx_port;


http {
    include       mime.types;
    default_type  application/octet-stream;

    #log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
    #                  '$status $body_bytes_sent "$http_referer" '
    #                  '"$http_user_agent" "$http_x_forwarded_for"';

    #access_log  logs/access.log  main;

    sendfile        on;
    #tcp_nopush     on;

    #keepalive_timeout  0;
    keepalive_timeout  65;

    #gzip  on;

    server {
        listen       %(port)d;
        server_name  localhost;

        #charset koi8-r;

        #access_log  logs/host.access.log  main;

        location / {
            root   html;
            index  index.html index.htm;
        }

location /nginx_status {
  # copied from http://blog.kovyrin.net/2006/04/29/monitoring-nginx-with-rrdtool/
  stub_status on;
  access_log   off;
  allow 127.0.0.1;
  deny all;
}

        #error_page  404              /404.html;

        # redirect server error pages to the static page /50x.html
        #
        error_page   500 502 503 504  /50x.html;
        location = /50x.html {
            root   html;
        }

        # proxy the PHP scripts to Apache listening on 127.0.0.1:80
        #
        #location ~ \.php$ {
        #    proxy_pass   http://127.0.0.1;
        #}

        # pass the PHP scripts to FastCGI server listening on 127.0.0.1:9000
        #
        #location ~ \.php$ {
        #    root           html;
        #    fastcgi_pass   127.0.0.1:9000;
        #    fastcgi_index  index.php;
        #    fastcgi_param  SCRIPT_FILENAME  /scripts$fastcgi_script_name;
        #    include        fastcgi_params;
        #}

        # deny access to .htaccess files, if Apache's document root
        # concurs with nginx's one
        #
        #location ~ /\.ht {
        #    deny  all;
        #}
    }


    # another virtual host using mix of IP-, name-, and port-based configuration
    #
    #server {
    #    listen       8000;
    #    listen       somename:8080;
    #    server_name  somename  alias  another.alias;

    #    location / {
    #        root   html;
    #        index  index.html index.htm;
    #    }
    #}


    # HTTPS server
    #
    #server {
    #    listen       443 ssl;
    #    server_name  localhost;

    #    ssl_certificate      cert.pem;
    #    ssl_certificate_key  cert.key;

    #    ssl_session_cache    shared:SSL:1m;
    #    ssl_session_timeout  5m;

    #    ssl_ciphers  HIGH:!aNULL:!MD5;
    #    ssl_prefer_server_ciphers  on;

    #    location / {
    #        root   html;
    #        index  index.html index.htm;
    #    }
    #}

}'''

def main():
  output_folder = 'configs/nginx/'
  num_servers = 100 # For fancies
  start_port = 16000
  for i in range(num_servers):
    port = i + start_port
    f = open(output_folder + 'nginx' + str(port) + '.conf', 'w')
    f.write(cfg_text  % {'port': port})
    f.close()

if __name__ == '__main__':
  main()

