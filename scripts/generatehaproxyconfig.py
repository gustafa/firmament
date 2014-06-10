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

def generate_server(hostname, max_rps, rps, port):
  MAX_WEIGHT = 256
  weight = int((rps / float(max_rps)) * MAX_WEIGHT)
  return '   server %(hostname)s%(port)d %(hostname)s:%(port)d weight %(weight)d disabled check' % \
      {'hostname': hostname, 'port': port, 'weight': weight}


def main():
  hostname_rpss = (('michael', 10608), ('uriel', 10610), ('pandaboard', 930), ('titanic',8222))
  start_port = 16000
  num_ports = 100
  max_rps = max([h[1] for h in hostname_rpss])
  servers = []

  for hostname_rps in hostname_rpss:
    hostname = hostname_rps[0]
    rps = hostname_rps[1]
    print hostname
    print rps
    servers.extend([generate_server(hostname, max_rps, rps, port + start_port) for port in range(num_ports)])

  f = open('configs/haproxy.cfg', 'w')
  f.write(cfg_text % {'servers': '\n'.join(servers)})


if __name__ == '__main__':
  main()
