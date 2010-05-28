#!/usr/bin/env python

import unittest
import test_context
import test_auth
import test_dir

if __name__ == '__main__':
    loader = unittest.TestLoader()
    suite1 = loader.loadTestsFromTestCase(test_context.TestContext)
    suite2 = loader.loadTestsFromTestCase(test_auth.TestAuth)
    suite3 = loader.loadTestsFromTestCase(test_dir.TestDir)
    alltests = unittest.TestSuite([suite1, suite2, suite3])
    unittest.TextTestRunner(verbosity=2).run(alltests)

