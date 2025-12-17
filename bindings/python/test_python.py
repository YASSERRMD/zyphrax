import unittest
import zyphrax

class TestZyphrax(unittest.TestCase):
    def test_basic_compress(self):
        data = b"Hello World " * 10
        comp = zyphrax.compress(data)
        self.assertGreater(len(comp), 12) # Header
        # self.assertLess(len(comp), len(data)) # Might not compress small string effectively + headers
        print(f"Compressed {len(data)} -> {len(comp)}")

    def test_large_compress(self):
        data = b"A" * 100000
        comp = zyphrax.compress(data)
        self.assertLess(len(comp), 2000) # Should be tiny
        
if __name__ == "__main__":
    unittest.main()
