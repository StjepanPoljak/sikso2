#!/usr/bin/env python3

import unittest
import subprocess
import os
import tempfile
import re
import textwrap

class Logger():

    @staticmethod
    def logi(message):
        print('(i) {}'.format(message))

    @staticmethod
    def logit(test, message):
        Logger.logi('[{}] {}'.format(test, message))

    @staticmethod
    def logt(message):
        print('   -> {}'.format(message))

    @staticmethod
    def exc(message):
        return '(!) {}'.format(message)

    @staticmethod
    def exct(message):
        return Logger.exc('[{}] {}'.format(test, message))

class Sikso2CPU():
    cpu_pre_re = re.compile('^.*Dumping CPU registers.*$')
    cpu_onl_re = re.compile('^(A): ([0-9a-f].) (X): ([0-9a-f].) '
                             '(Y): ([0-9a-f].) (S): ([0-9a-f].) '
                             '(P): ([0-9a-f].) (PC): ([0-9a-f]...)$')

    def __init__(self):
        self.match = None
        self.cpu_data = {}

    @staticmethod
    def cpu_data_next(line):
        return True if Sikso2CPU.cpu_pre_re.match(line) else False

    def get_cpu_data(self, line):
        self.match = Sikso2CPU.cpu_onl_re.match(line)

        if not self.match:
            return

        if self.cpu_data:
            raise Exception(Logger.exc('CPU data already filled in.'))

        for each in range(1, 7):
            self.cpu_data[self.match.group(2 * each - 1)] \
                    = int(self.match.group(2 * each), 16)

        return

    def __del__(self):
        Logger.logi('Closing Sikso2Code')

class Sikso2Code():

    error_re = re.compile('^\[[A-Z]..\] \(!\) (.*)$')
    leak_re = re.compile('^==[0-9]+== All heap blocks were freed -- no leaks are possible$')

    def __init__(self, name, contents, args=[], leak_check=False):
        self.name = name
        self.contents = contents
        if args:
            self.args = args
        else:
            self.args = []
        self.sikso2cpu = Sikso2CPU()
        self.got_cpu_data = False
        self.res = None
        self.leak_check = leak_check

        Logger.logit(name, 'Translating code:\n{}'.format(
            textwrap.indent(self.contents, '\t'))
        )

    def find_cpu_data(self):

        for line in self.res:

            if self.got_cpu_data:
                self.sikso2cpu.get_cpu_data(line)
            elif Sikso2CPU.cpu_data_next(line):
                    self.got_cpu_data = True

        if not self.got_cpu_data:
            raise Exception(Logger.exct(self.name, 'Could not find CPU data.'))

    def check_for_errors(self, raise_exc=False):
        match = None

        for line in self.res:
            match = Sikso2Code.error_re.match(line)

            if match:
                if raise_exc:
                    raise Exception('Found unexpected error: '
                                    '"{}"'.format(match.group(1)))
                else:
                    return match.group(1)

        return ''

    def check_for_leaks(self):
        match = None

        if not self.leak_check:
            raise Exception(Logger.exc('Not configured for leak check.'))

        for line in self.res:
            match = Sikso2Code.leak_re.match(line)

            if match:
                raise Exception(Logger.exc('Found leaks.'))

    def find_mem_data(self, res):

        return

    @staticmethod
    def make():
        subprocess.run(['make', 'clean'], capture_output=True)
        subprocess.run(['make', 'CONFIG_FILE=tests/config.txt'],
                capture_output=True)

    def run(self):
        fd, path = tempfile.mkstemp()
        mandatory = ['./sikso2', '-r', path, '-s', '-d', 'oneline']
        full_run = (['valgrind'] if self.leak_check else []) \
                 + mandatory + self.args

        try:
            with os.fdopen(fd, 'w') as tmp:
                tmp.write(self.contents)

        finally:
            self.res = subprocess.run(full_run,
                capture_output=True, text=True
            ).stdout.split('\n')

            Logger.logi('Running:')
            Logger.logt(" ".join(full_run))

            os.remove(path)

class Sikso2Tests(unittest.TestCase):
    """ sikso2 tests """

    def setUp(self):
        Sikso2Code.make()

    def assertCPURegisterEqual(self, sikso2code, reg, val):
        Logger.logt('Checking if register {} '
                    'equals {} ({})...'.format(reg, hex(val), val))
        self.assertEqual(sikso2code.sikso2cpu.cpu_data[reg], val)

    def assertCPUStatusBitsSet(self, sikso2code, bits):
        Logger.logt('Checking if status bits {} are set...'.format(bits))
        for bit in bits:
            self.assertNotEqual(sikso2code.sikso2cpu.cpu_data['P'] & (1 << bit), 0)

    def assertCPUStatusBitsClear(self, sikso2code, bits):
        Logger.logt('Checking if status bits {} are clear...'.format(bits))
        for bit in bits:
            self.assertEqual(sikso2code.sikso2cpu.cpu_data['P'] & (1 << bit), 0)

    def assertFoundError(self, sikso2code, err_msg):
        Logger.logt('Checking if "{}" error is found...'.format(err_msg))
        err = sikso2code.check_for_errors()
        self.assertTrue(err_msg in err)

    def assertNoLeaks(self, sikso2code):
        Logger.logt('Checking for memory leaks...')
        sikso2code.check_for_leaks()

    def test1_integrity(self):
        s2c = Sikso2Code('integrity-m1', 'NOP', ['-m', '0x0600-'],
                leak_check=True)
        s2c.run()
        self.assertFoundError(s2c, 'Invalid argument')
        self.assertNoLeaks(s2c)

        s2c = Sikso2Code('integrity-m2', 'NOP', ['-m', '0x0600-ggg'],
                leak_check=True)
        s2c.run()
        self.assertFoundError(s2c, 'Invalid argument')
        self.assertNoLeaks(s2c)

        s2c = Sikso2Code('integrity-m4', 'NOP', ['-m', '0x0600,ggg'],
                leak_check=True)
        s2c.run()
        self.assertFoundError(s2c, 'Invalid argument')
        self.assertNoLeaks(s2c)

        _, path = tempfile.mkstemp()
        os.remove(path)
        s2c = Sikso2Code('integrity-f1', 'NOP',
                ['-f', '0x0700:{}'.format(path)], leak_check=True)
        s2c.run()
        self.assertFoundError(s2c, 'Could not load {}.'.format(path))
        self.assertNoLeaks(s2c)

        s2c = Sikso2Code('integrity-f2', 'NOP', ['-f', '0x0700:'],
            leak_check=True)
        s2c.run()
        self.assertFoundError(s2c, 'Invalid argument')
        self.assertNoLeaks(s2c)

        s2c = Sikso2Code('integrity-f3', 'NOP', ['-f', 'ggg'], leak_check=True)
        s2c.run()
        self.assertFoundError(s2c, 'Invalid argument')
        self.assertNoLeaks(s2c)

        s2c = Sikso2Code('integrity-fm', 'NOP', ['-m', '0x0600-', '-f', 'ggg'],
            leak_check=True)
        s2c.run()
        self.assertFoundError(s2c, 'Invalid argument')
        self.assertNoLeaks(s2c)

    def test2_lda(self):
        print('')
        s2c = Sikso2Code('test_lda', 'LDA #$05')
        s2c.run()
        s2c.find_cpu_data()
        self.assertCPURegisterEqual(s2c, 'A', 5)
        self.assertCPUStatusBitsSet(s2c, [5])
        self.assertCPUStatusBitsClear(s2c, [0, 1, 2, 3, 4, 6, 7])

        s2c = Sikso2Code('test_lda (Z)', 'LDA #$00')
        s2c.run()
        s2c.find_cpu_data()
        self.assertCPURegisterEqual(s2c, 'A', 0)
        self.assertCPUStatusBitsSet(s2c, [1, 5])
        self.assertCPUStatusBitsClear(s2c, [0, 2, 3, 4, 6, 7])

        s2c = Sikso2Code('test_lda (N)', 'LDA #$80')
        s2c.run()
        s2c.find_cpu_data()
        self.assertCPURegisterEqual(s2c, 'A', int("80", 16))
        self.assertCPUStatusBitsSet(s2c, [5, 7])
        self.assertCPUStatusBitsClear(s2c, [0, 1, 2, 3, 4, 6])

    def test3_adc(self):
        print('')
        s2c = Sikso2Code('test_adc', 'LDA #$05\nADC #$07')
        s2c.run()
        s2c.find_cpu_data()
        self.assertCPURegisterEqual(s2c, 'A', 12)

        s2c = Sikso2Code('test_adc (C)', 'LDA #$82\nADC #$7f')
        s2c.run()
        s2c.find_cpu_data()
        self.assertCPURegisterEqual(s2c, 'A', 1)
        self.assertCPUStatusBitsSet(s2c, [0, 5])

        s2c = Sikso2Code('test_adc (V)', 'LDA #$8f\nADC #$82')
        s2c.run()
        s2c.find_cpu_data()
        self.assertCPURegisterEqual(s2c, 'A', int("11", 16))
        self.assertCPUStatusBitsSet(s2c, [0, 5, 6])

unittest.main()
