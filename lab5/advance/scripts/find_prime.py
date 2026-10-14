sieve = [0] * 65536
prime = []

sieve[1] = 1

for i in range(2, 65536):
	if not sieve[i]:
		prime.append(i)
	j = 0
	while i * prime[j] < 65536:
		sieve[i*prime[j]] = 1
		if i % prime[j] == 0:
			break
		j = j+1

print("PSECT data")
print("prime_table:")

for p in prime:
    print(f"\tDB 0x{p & 0xFF:02X}, 0x{p >> 8:02X}")

print("\tDB 0x00, 0x00")