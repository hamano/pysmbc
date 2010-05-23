#!/usr/bin/env python

import unittest
import test_context
import test_auth

if __name__ == '__main__':
    loader = unittest.TestLoader()
    suite1 = loader.loadTestsFromTestCase(test_context.TestContext)
    suite2 = loader.loadTestsFromTestCase(test_auth.TestAuth)
    alltests = unittest.TestSuite([suite1, suite2])
    unittest.TextTestRunner(verbosity=2).run(alltests)

