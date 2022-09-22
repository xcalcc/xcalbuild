#!/usr/bin/env python3

import argparse
import json
import sys
import time
import requests

def get_token(args):
    result = requests.post(args.api + '/api/auth_service/v2/login', json={"username": args.user, "password": args.password}, headers={})
    retry = 0
    while retry < 5:
        if result is None:
            print("server cannot be reached, please check whether xcal-server is available or network is reachable")
            sys.exit(1)
        elif 400 <= result.status_code < 500:
            print("login failed, error message: %s" % result.content)
            print("login failed, please check whether xcal-server is available or your username password is correct")
            sys.exit(1)
        elif 500 <= result.status_code < 600:
            print("login failed, error message: %s" % result.content)
            print("login failed, please check whether xcal-server is available")
            time.sleep(5)
            retry += 1
            continue
        else:
            return result.json().get("accessToken")
    return ""

def parse_args():
    parser = argparse.ArgumentParser(description="fastAgent for client-side prescan operation")
    parser.add_argument("-i", "--api",
                        help="api server url, like http://127.0.0.1:80")
    parser.add_argument("-u", "--user",
                        help="server username")
    parser.add_argument("-p", "--password",
                        help="server password")
    return parser.parse_args()

args = parse_args()
token = get_token(args)
print(token)
