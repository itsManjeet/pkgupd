#!/bin/env python

import os
import sys
import yaml
import re

IN_RECIPE_FILE = sys.argv[1]
OUT_RECIPE_FILE = sys.argv[2]

with open(IN_RECIPE_FILE, 'r') as file:
    data = yaml.safe_load(file)

if len(data['packages']) != 1:
    print("no supported for multiple packages, skipping {}".format(IN_RECIPE_FILE))
    exit(1)

package = data['packages'][0]

sources = []
if 'sources' in data:
    sources += data['sources']
if 'sources' in package:
    sources += package['sources']

build_depends = []
runtime_depends = []
if 'depends' in data:
    if 'buildtime' in data['depends']:
        build_depends += data['depends']['buildtime']
    if 'runtime' in data['depends']:
        runtime_depends += data['depends']['runtime']

if 'depends' in package:
    if 'buildtime' in package['depends']:
        build_depends += package['depends']['buildtime']
    if 'runtime' in package['depends']:
        runtime_depends += package['depends']['runtime']


new_data = {
    'id': data['id'],
    'version': data['version'],
    'about': data['about'].split('\n')[0],
    'sources': sources,
    'build-dir': package['dir'],
}

if 'pack' in package:
    new_data['type'] = package['pack']

if len(build_depends) != 0 or len(runtime_depends) != 0:
    new_data["depends"] = {}

    if len(build_depends) != 0:
        new_data["depends"]["buildtime"] = build_depends
    
    if len(runtime_depends) != 0:
        new_data["depends"]["runtime"] = runtime_depends

if 'postscript' in package:
    new_data['post-script'] = package['postscript']

if 'prescript' in package:
    new_data['pre-script'] = package['prescript']

if 'script' in package:
    new_data['script'] = package['script']

if 'users' in data:
    new_data['users'] = data['users']
if 'groups' in data:
    new_data['groups'] = data['groups']

environ = []
if 'environ' in data:
    environ += data['environ']
if 'environ' in package:
    environ += package['environ']
if len(environ) != 0:
    new_data['environ'] = environ

def checkflag(id: str) -> None:
    if 'flags' in package:
        for i in package['flags']:
            if i['id'] == id:
                new_data[id] = i['value']
                return

checkflag('configure')
checkflag('compile')
checkflag('install')

with open(OUT_RECIPE_FILE, 'w+') as file:
    file.write(yaml.safe_dump(new_data, sort_keys=False))
