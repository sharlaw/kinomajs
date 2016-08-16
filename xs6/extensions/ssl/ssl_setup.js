/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
import SSL from "ssl";
import PRF from "ssl/prf";
import SSLStream from "ssl/stream";
import Crypt from "crypt";

function setupSub(o, cipher)
{
	switch (cipher.cipherAlgorithm) {
	case SSL.cipherSuite.DES:
		var enc = new Crypt.DES(o.key);
		break;
	case SSL.cipherSuite.TDES:
		var enc = new Crypt.TDES(o.key);
		break;
	case SSL.cipherSuite.AES:
		var enc = new Crypt.AES(o.key);
		break;
	case SSL.cipherSuite.RC4:
		var enc = new Crypt.RC4(o.key);
		break;
	default:
		throw new Error("SSL: SetupCipher: unkown encryption algorithm");
	}
	switch (cipher.encryptionMode) {
	case SSL.cipherSuite.CBC:
	case SSL.cipherSuite.NONE:
		let h;
		switch (cipher.hashAlgorithm) {
		case SSL.cipherSuite.MD5: h = new Crypt.MD5(); break;
		case SSL.cipherSuite.SHA1: h = new Crypt.SHA1(); break;
		case SSL.cipherSuite.SHA256: h = new Crypt.SHA256(); break;
		case SSL.cipherSuite.SHA384: h = new Crypt.SHA384(); break;
		default:
			throw new Error("SSL: SetupCipher: unknown hash algorithm");
		}
		o.hmac = new Crypt.HMAC(h, o.macSecret);
		if (cipher.encryptionMode == SSL.cipherSuite.CBC)
			o.enc = new Crypt.CBC(enc, o.iv);	// no padding -- SSL 3.2 requires padding process beyond RFC2630
		else
			o.enc = enc;
		break;
	case SSL.cipherSuite.GCM:
		let Arith = require.weak("arith");
		o.enc = new Crypt.GCM(enc);
		o.nonce = new Arith.Integer(1);
		break;
	default:
		o.enc = enc;
		break;
	}
}

function SetupCipher(session, connectionEnd)
{
	var random = session.serverRandom;
	random = random.concat(session.clientRandom);
	var chosenCipher = session.chosenCipher;
	var macSize = chosenCipher.encryptionMode == SSL.cipherSuite.GCM ? 0 : chosenCipher.hashSize;
	var ivSize = session.protocolVersion <= 0x301 ? chosenCipher.cipherBlockSize : (chosenCipher.saltSize || 0);
	var nbytes = chosenCipher.cipherKeySize * 2 + macSize * 2 + ivSize * 2;
	var keyBlock = PRF(session, session.masterSecret, "key expansion", random, nbytes);
	var s = new SSLStream(keyBlock);
	var o = {};
	if (connectionEnd) {
		if (macSize > 0) {
			o.macSecret = s.readChunk(macSize);
			void s.readChunk(macSize);
		}
		o.key = s.readChunk(chosenCipher.cipherKeySize);
		void s.readChunk(chosenCipher.cipherKeySize);
		if (ivSize > 0)
			o.iv = s.readChunk(ivSize);
		else
			o.iv = undefined;
		setupSub(o, chosenCipher);
		session.clientCipher = o;
	}
	else {
		if (macSize > 0) {
			void s.readChunk(macSize);
			o.macSecret = s.readChunk(macSize);
		}
		void s.readChunk(chosenCipher.cipherKeySize);
		o.key = s.readChunk(chosenCipher.cipherKeySize);
		if (ivSize > 0) {
			void s.readChunk(ivSize);
			o.iv = s.readChunk(ivSize);
		}
		else
			o.iv = undefined;
		setupSub(o, chosenCipher);
		session.serverCipher = o;
	}
}

export default SetupCipher;
