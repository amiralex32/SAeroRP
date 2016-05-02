# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('aerorp', ['network','core','internet','mobility'])
    module.source = [
        'model/aerorp-pqueue.cc',
        'model/aerorp-ptable.cc',
        'model/aerorp-ntable.cc',
        'model/aerorp-packet.cc',
        'model/aerorp-routing-protocol.cc',
        'model/certificate-authority.cc',
        'model/gcm-converter.cc',
        'helper/aerorp-helper.cc',


        ]

    headers = bld.new_task_gen(features=['ns3header'])
    headers.module = 'aerorp'
    headers.source = [
        'model/aerorp-pqueue.h',
        'model/aerorp-ptable.h',
        'model/aerorp-ntable.h',
        'model/aerorp-packet.h',
        'model/aerorp-routing-protocol.h',
        'model/certificate-authority.h',
        'model/gcm-converter.h',
        'helper/aerorp-helper.h',

        ]

    bld.ns3_python_bindings()

