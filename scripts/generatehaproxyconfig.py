#!/usr/bin/python2

cfg_text = '''global
    daemon
    maxconn 256
    stats socket /home/gjrh2/haproxy.socket level admin
    pidfile /tmp/haproxy.pid

defaults
    mode http
    timeout connect 5000ms
    timeout client 50000ms
    timeout server 50000ms

frontend http-in
    bind *:80
    default_backend my_servers

backend my_servers
%(servers)s

listen stats :9039
    mode http
    stats enable
    stats hide-version
    stats realm Haproxy\ Statistics
    stats uri /
'''

def generate_server(hostname, port):
  return '   server %(hostname)s%(port)d %(hostname)s:%(port)d disabled check' % \
      {'hostname': hostname, 'port': port}


def main():
  hostnames = ['michael', 'uriel', 'pandaboard', 'titanic']
  start_port = 16000
  num_ports = 100
  servers = []

  for hostname in hostnames:
    servers.extend([generate_server(hostname, port + start_port) for port in range(num_ports)])

  f = open('configs/haproxy.cfg', 'w')
  f.write(cfg_text % {'servers': '\n'.join(servers)})


if __name__ == '__main__':
  main()
