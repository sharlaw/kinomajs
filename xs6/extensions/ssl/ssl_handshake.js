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
import Crypt from "crypt";
import SSL from "ssl";
import SSLProtocol from "ssl/protocol";
import SSLStream from "ssl/stream";
import Bin from "bin";

const hello_request = 0;
const client_hello = 1;
const server_hello = 2;
const certificate = 11;
const server_key_exchange = 12;
const certificate_request = 13;
const server_hello_done = 14;
const certificate_verify = 15,
const client_key_exchange = 16,
const finished = 20;

const master_secret_label = "master secret";
const client_finished_label = "client finished";
const server_finished_label = "server finished";

const MD5 = 1;
const SHA1 = 2;
const SHA256 = 4;

const extension_type = {
	tls_server_name: 0,
	tls_max_fragment_length: 1,
	tls_client_certification_url: 2,
	tls_trusted_ca_keys: 3,
	tls_trusted_hmac: 4,
	tls_status_request: 5,
	tls_signature_algorithms: 13,
	tls_application_layer_protocol_negotiation: 16,
};

function handshakeDigestUpdate(session, msg)
{
	if (!session.handshakeMessages)
		session.handshakeMessages = new SSLStream();
	session.handshakeMessages.writeChunk(msg);
}

function handshakeDigestResult(session, which)
{
	var msg = session.handshakeMessages.getChunk(), H;
	if (which & MD5) {
		var md5 = new Crypt.MD5();
		H = md5.process(msg);
	}
	if (which & SHA1) {
		var sha1 = new Crypt.SHA1();
		var res = sha1.process(msg);
		H = H ? H.concat(res) : res;
	}
	if (which & SHA256) {
		var sha256 = new Crypt.SHA256();
		var res = sha256.process(msg);
		H = H ? H.concat(res) : res;
	}
	return H;
}

var handshakeProtocol = {
	name: "handshakeProtocol",

	unpacketize(session, s) {
		session.traceProtocol(this);
		var msgType = s.readChar(0);
		var bodyLen = s.readChars(3);
		var body = bodyLen > 0 ? s.readChunk(bodyLen) : undefined;
		var protocol;
		switch (msgType) {
		case hello_request:
			protocol = this.helloRequest;
			break;
		case client_hello:
			protocol = this.clientHello;
			break;
		case server_hello:
			protocol = this.serverHello;
			break;
		case certificate:
			protocol = this.certificate;
			break;
		case server_key_exchange:
			protocol = this.serverKeyExchange;
			break;
		case certificate_request:
			protocol = this.certificateRequest;
			break;
		case server_hello_done:
			protocol = this.serverHelloDone;
			break;
		case certificate_verify:
			protocol = this.certificateVerify;
			break;
		case client_key_exchange:
			protocol = this.clientKeyExchange;
			break;
		case finished:
			protocol = this.finished;
			break;
		default:
			throw new Error("SSL: handshake: unknown type");
			break;
		}
		protocol.unpacketize(session, body ? new SSLStream(body) : undefined);
		// update the digest of the whole handshake message
		var tbuf = new Uint8Array(4);
		tbuf[0] = msgType;
		for (var i = 0; i < 3; i++)
			tbuf[i + 1] = (bodyLen >>> (8 * (2 - i))) & 0xff;
		handshakeDigestUpdate(session, tbuf.buffer);
		if (body)
			handshakeDigestUpdate(session, body);
		session.handshakeProcess = msgType;
	},
	packetize(session, msgType, body) {
		session.traceProtocol(this);
		var s = new SSLStream();
		s.writeChar(msgType);
		s.writeChars(body.bytesWritten, 3);
		s.writeChunk(body.getChunk());
		var msg = s.getChunk();
		handshakeDigestUpdate(session, msg);
		return SSLProtocol.recordProtocol.packetize(session, SSLProtocol.recordProtocol.handshake, msg);
	},

	// protocols
	helloRequest: {
		name: "helloRequest",
		msgType: hello_request,
		unpacketize(session) {
			session.traceProtocol(this);
			// nothing to unpacketize
		},
		packetize(session) {
			// should not be called
			debugger;
		},
	},

	helloProtocol: {
		random: {
			parse(s) {
				var t = s.readChars(4);
				return {
					qmt_unix_time: t,
					random_bytes: s.readChunk(28)
				};
			},
			serialize() {
				var s = new SSLStream();
				var qmt_unix_time = (new Date()).getTime() / 1000;	// in sec
				var random_bytes = Crypt.rng(28);
				s.writeChars(qmt_unix_time, 4);
				s.writeChunk(random_bytes);
				return s.getChunk();
			},
		},
		selectCipherSuite(peerSuites) {
			for (var i = 0, suites = SSL.supportedCipherSuites; i < suites.length; i++) {
				var mySuite = suites[i].value;
				for (var j = 0; j < peerSuites.length; j++) {
					if (mySuite[0] == peerSuites[j][0] && mySuite[1] == peerSuites[j][1])
						return suites[i];
				}
			}
			throw new Error("SSL: handshake: unsuppoorted cipher");
		},
		selectCompressionMethod(peerMethods) {
			for (var i = 0, methods = SSL.supportedCompressionMethods; i < methods.length; i++) {
				for (var j = 0; j < peerMethods.length; j++) {
					if (methods[i] == peerMethods[j])
						return methods[i];
				}
			}
			throw new Error("SSL: handshake: unsupported compression");
		},
		unpacketize(session, s, msgType) {
			var ver = s.readChars(2);
			if (ver < session.minProtocolVersion) {
				session.alert = {level: 2, description: 70};
				return;
			}
			else if (ver > session.maxProtocolVersion)
				session.protocolVersion = session.maxProtocolVersion;
			else
				session.protocolVersion = ver;	// should support all versions between [min, max]
			var random = s.readChunk(32);
			var sessionIDLen = s.readChar();
			var sessionID = sessionIDLen > 0 ? s.readChunk(sessionIDLen) : undefined;
			var suites = [];
			var compressionMethods = [];
			if (msgType == client_hello) {
				session.clientRandom = random;
				session.clientSessionID = sessionID;
				for (var nsuites = s.readChars(2) / 2; --nsuites >= 0;) {
					var c1 = s.readChar();
					var c2 = s.readChar();
					suites.push([c1, c2]);
				}
				// select the most suitable one
				for (var nmethods = s.readChar(); --nmethods >= 0;)
					compressionMethods.push(s.readChar());
			}
			else {
				session.serverRandom = random;
				session.serverSessionID = sessionID;
				var c1 = s.readChar();
				var c2 = s.readChar();
				suites.push([c1, c2]);
				compressionMethods.push(s.readChar());
			}
			session.chosenCipher = this.selectCipherSuite(suites);
			session.compressionMethod = this.selectCompressionMethod(compressionMethods);
			if (msgType == server_hello && s.byteAvailable) {
				let type = s.readChars(2);
				switch (type) {
				case extension_type.tls_signature_algorithms:
					let len = s.readChars(2);
					let n = s.readChars(2);
					session.signature_algorithms = [];
					for (let i = 0; i < n; i++) {
						let hash = s.readChar();
						let sig = s.readChar();
						session.signature_algorithms.push({hash, sig});
					}
					break;
				default:
					break;
				}
			}
		},
		packetize(session, cipherSuites, compressionMethods, msgType) {
			var s = new SSLStream();
			s.writeChars(session.protocolVersion, 2);
			var random = this.random.serialize();
			s.writeChunk(random);
			if (msgType == client_hello) {
				session.clientRandom = random;
				var sessionID = session.clientSessionID;
			}
			else {
				session.serverRandom = random;
				var sessionID = session.serverSessionID;
			}
			if (!sessionID || !sessionID.byteLength)
				s.writeChar(0);
			else {
				s.writeChar(sessionID.byteLength);
				s.writeChunk(sessionID);
			}
			if (msgType == client_hello) {
				s.writeChars(cipherSuites.length * 2, 2);
				for (var i = 0; i < cipherSuites.length; i++) {
					var val = cipherSuites[i].value;
					s.writeChar(val[0]);
					s.writeChar(val[1]);
				}
				s.writeChar(compressionMethods.length);
				for (var i = 0; i < compressionMethods.length; i++)
					s.writeChar(compressionMethods[i]);
				//
				// TLS extensions
				//
				var es = new SSLStream();
				for (var i in session.options) {
					if (!(i in extension_type))
						continue;
					var type = extension_type[i];
					var ext = session.options[i];
					switch (type) {
					case extension_type.tls_server_name:
						es.writeChars(type, 2);
						var len = 1 + 2 + ext.length;
						es.writeChars(2 + len, 2);
						es.writeChars(len, 2);
						es.writeChar(0);		// name_type, 0 -- host_name
						es.writeChars(ext.length, 2);
						es.writeString(ext);
						break;
					case extension_type.tls_max_fragment_length:
						es.writeChars(type, 2);
						es.writeChars(2 + 1, 2);
						es.writeChars(1, 2);
						var j;
						for (j = 1; j <= 4; j++) {
							var e = j + 9;	// start with 2^9
							if ((ext >>> e) == 0)
								break;
						}
						if (j > 4)
							j = 4;
						es.writeChar(j);
						break;
					case extension_type.tls_signature_algorithms:
						es.writeChars(type, 2);
						es.writeChars(2 + ext.length * 2, 2);
						es.writeChars(ext.length, 2);
						for (let j = 0; j < ext.length; j++) {
							es.writeChar(ext[j].hash);
							es.writeChar(ext[j].sig);
						}
						break;
					case extension_type.tls_application_layer_protocol_negotiation:
						es.writeChars(type, 2);
						var len = 0;
						if (typeof ext == 'string') ext = ext.split(':');
						for (var j = 0; j < ext.length; j++)
							len += ext[j].length + 1;
						es.writeChars(len + 2, 2);
						es.writeChars(len, 2);
						for (var j = 0; j < ext.length; j++) {
							var name = ext[j];
							es.writeChars(name.length, 1);
							es.writeString(name);
						}
						break;
					default:
						// not supported yet
						break;
					}
				}
				if (es.bytesAvailable) {
					s.writeChars(es.bytesAvailable, 2);
					s.writeChunk(es.getChunk());
				}
			}
			else {
				var val = cipherSuites[0].value;
				s.writeChar(val[0]);
				s.writeChar(val[1]);
				s.writeChar(compressionMethods[0]);
			}
			return handshakeProtocol.packetize(session, msgType, s);
		},
	},

	clientHello: {
		name: "clientHello",
		msgType: client_hello,
		unpacketize(session, s) {
			session.traceProtocol(this);
			handshakeProtocol.helloProtocol.unpacketize(session, s, this.msgType);
		},
		packetize(session) {
			session.traceProtocol(this);
			return handshakeProtocol.helloProtocol.packetize(session, SSL.supportedCipherSuites, SSL.supportedCompressionMethods, this.msgType);
		},
	},

	serverHello: {
		name: "serverHello",
		msgType: server_hello,
		unpacketize(session, s) {
			session.traceProtocol(this);
			handshakeProtocol.helloProtocol.unpacketize(session, s, this.msgType);
		},
		packetize(session) {
			session.traceProtocol(this);
			return handshakeProtocol.helloProtocol.packetize(session, [session.chosenCipher], [session.compressionMethod], this.msgType);
		},
	},

	certificate: {
		name: "certificate",
		msgType: certificate,
		matchName(re, name) {
			re = re.replace(/\./g, "\\.").replace(/\*/g, "[^.]*");
			var a = name.match(new RegExp("^" + re + "$", "i"));
			return a && a.length == 1;
		},
		verifyHost(session, cert) {
			var hostname = session.socket.host;
			var altNames = Crypt.X509.getExtension(cert, 'subjectAlternativeName');
			for (var i = 0; i < altNames.length; i++) {
				var name = altNames[i];
				if (typeof altNames[i] == "string" && this.matchName(name, hostname))
					return true;
			}
			// @@ not supporting the common name
			// var arr = Crypt.X509.decodeTBS(cert).subject.match(/CN=([^,]*)/);
			// return arr && arr.length > 1 && this.matchName(arr[1], hostname);
		},
		unpacketize(session, s) {
			session.traceProtocol(this);
			var certs = [];
			var ttlSize = s.readChars(3);
			while (ttlSize > 0 && s.bytesAvailable > 0) {
				var certSize = s.readChars(3);
				certs.push(s.readChunk(certSize));
				ttlSize -= certSize + 3;
			}
			if (!session.certificateManager.verify(certs, session.options))
				throw new Error("SSL: certificate: auth err");
			if (session.options.verifyHost) {
				if (!this.verifyHost(session, certs[0]))
					throw new Error("SSL: certificate: bad host");
			}
			session.peerCert = certs[0];
			session.certificateManager.register(session.peerCert);
		},
		packetize(session, certs) {
			session.traceProtocol(this);
			var s = new SSLStream();
			// calculate the length
			var len = 0;
			for (var i = 0; i < certs.length; i++)
				len += certs[i].byteLength;
			len += 3;
			s.writeChars(len, 3);
			for (var i = 0; i < certs.length; i++) {
				var c = certs[i];
				s.writeChars(c.byteLength, 3);
				s.writeChunk(c);
			}
			if (certs.length > 0)
				session.myCert = certs[0];	// the first one
			return handshakeProtocol.packetize(session, certificate, s);
		},
	},

	serverKeyExchange: {
		name: "serverKeyExchange",
		msgType: server_key_exchange,
		// hash algorithms
		none: 0,
		md5: 1,
		sha1: 2,
		sha224: 3,
		sha256: 4,
		sha384: 5,
		sha512: 6,
		// signature algorithms
		anonymous: 0,
		rsa: 1,
		dsa: 2,
		ecdsa: 3,
		
		unpacketize(session, s) {
			session.traceProtocol(this);
			switch (session.chosenCipher.keyExchangeAlgorithm) {
			case SSL.cipherSuite.DHE_DSS:
			case SSL.cipherSuite.DHE_RSA:
				let tbs = new SSLStream();
				let dhparams = {};
				let n = s.readChars(2);
				tbs.writeChars(n, 2);
				dhparams.dh_p = s.readChunk(n);
				tbs.writeChunk(dhparams.dh_p);
				n = s.readChars(2);
				tbs.writeChars(n, 2);
				dhparams.dh_g = s.readChunk(n);
				tbs.writeChunk(dhparams.dh_g);
				n = s.readChars(2);
				tbs.writeChars(n, 2);
				dhparams.dh_Ys = s.readChunk(n);
				tbs.writeChunk(dhparams.dh_Ys);
				session.dhparams = dhparams;
				let hash_algo = s.readChar();
				let sig_algo = s.readChar();
				n = s.readChars(2);
				let sig = s.readChunk(n);
				let hash, pk;
				switch (hash_algo) {
				default:
				case this.none: break;
				case this.md5: hash = Crypt.MD5; break;
				case this.sha1: hash = Crypt.SHA1; break;
				case this.sha224: hash = Crypt.SHA224; break;
				case this.sha256: hash = Crypt.SHA256; break;
				case this.sha384: hash = Crypt.SHA384; break;
				case this.sha512: hash = Crypt.SHA512; break;
				}
				switch (sig_algo) {
				default:
				case this.anonymous: break;
				case this.rsa: pk = Crypt.PKCS1_5; break;
				case this.dsa: pk = Crypt.DSA; break;
				case this.ecdsa: pk = Crypt.ECDSA; break;
				}
				if (hash && pk && sig) {
					let H = (new hash()).process(session.clientRandom, session.serverRandom, tbs.getChunk());
					let key = session.certificateManager.getKey(session.peerCert);
					let v = new pk(key, false, [] /* any oid for PKCS1_5 */);
					if (!v.verify(H, sig)) {
						// should send an alert, probably...
						throw new Error("SSL: serverKeyExchange: failed to verify signature");
					}
				}
				hash = pk = sig = tbs = null;
				break;
			case SSL.cipherSuite.RSA:
				// no server key exchange info
				break;
			case SSL.cipherSuite.DH_ANON:
			case SSL.cipherSuite.DH_DSS:
			case SSL.cipherSuite.DH_RSA:
			default:
				// not supported
				throw new Error("SSL: serverKeyExchange: unsupported algorithm: " + algo);
				break;
			}
		},
		packetize(session) {
			let pkt;
			session.traceProtocol(this);
			switch (session.chosenCipher.keyExchangeAlgorithm) {
			case SSL.cipherSuite.DHE_DSS:
			case SSL.cipherSuite.DHE_RSA:
			case SSL.cipherSuite.DH_ANON:
				let s = new SSLStream();
				let Arith = require.weak("arith");
				let dh = session.certificateManager.getDH();
				let mod = new Arith.Mont({z: new Arith.Z(), m: dh.p});
				dh.x = new Arith.Integer(Crypt.rng(dh.p.sizeof()));
				let y = mod.exp(dh.g, dh.x);
				let tbs = new SSLStream(), c;
				// server DH params
				c = dh.p.toChunk();
				tbs.writeChars(c.byteLength, 2);
				tbs.writeChunk(c);
				c = dh.g.toChunk();
				tbs.writeChars(c.byteLength, 2);
				tbs.writeChunk(c);
				c = y.toChunk();
				tbs.writeChars(c.byteLength, 2);
				tbs.writeChunk(c);
				s.writeChunk(tbs.getChunk());
				if (session.chosenCipher.keyExchangeAlgorithm != SSL.cipherSuite.DH_ANON) {
					// digitally-signed
					let hash_algo = -1, sig_algo = -1, hash, pk, oid;
					if (session.signature_algorithms && session.signature_algorithms.length > 0) {
						// take the first one
						hash_algo = session.signature_algorithms[0].hash;
						sig_algo = session.signature_algorithms[0].sig;
					}
					else {
						hash_algo = this.sha1;
						sig_algo = session.chosenCipher.keyExchangeAlgorithm == SSL.cipherSuite.DHE_RSA ? this.rsa : this.dsa;
					}
					switch (hash_algo) {
					default:
					case this.none: throw new Error("SSL: serverKeyExchange: no hash algorithm"); break;
					case this.sha1: hash = Crypt.SHA1; oid = [1, 3, 14, 3, 2, 26]; break;
					case this.md5: hash = Crypt.MD5; oid = [1, 2, 840, 113549, 2, 5]; break;
					case this.sha224: hash = Crypt.SHA224; oid = [2, 16, 840, 1, 101, 3, 4, 2, 4]; break;
					case this.sha256: hash = Crypt.SHA256; oid = [2, 16, 840, 1, 101, 3, 4, 2, 1]; break;
					case this.sha384: hash = Crypt.SHA384; oid = [2, 16, 840, 1, 101, 3, 4, 2, 2]; break;
					case this.sha512: hash = Crypt.SHA512; oid = [2, 16, 840, 1, 101, 3, 4, 2, 3]; break;
					}
					switch (sig_algo) {
					default:
					case this.anonymous: throw new Error("SSL: serverKeyExchange: no signature algorithm"); break;
					case this.rsa: pk = Crypt.PKCS1_5; break;
					case this.dsa: pk = Crypt.DSA; break;
					case this.ecdsa: pk = Crypt.ECDSA; break;
					}
					let key = session.certificateManager.getKey(/* self */);
					let sig = new pk(key, true, oid);
					let H = (new hash()).process(session.clientRandom, session.serverRandom, tbs.getChunk());
					let signature = sig.sign(H);
					s.writeChar(hash_algo);
					s.writeChar(sig_algo);
					s.writeChars(signature.byteLength, 2);
					s.writeChunk(signature);
				}
				session.dh = dh;
				pkt = handshakeProtocol.packetize(session, server_key_exchange, s);
				break;
			case SSL.cipherSuite.RSA:
				// no server key exchange info
				break;
			case SSL.cipherSuite.DH_DSS:
			case SSL.cipherSuite.DH_RSA:
				pkt = handshakeProtocol.packetize(session, server_key_exchange, new SSLStream());	// empty body
				break;
			default:
				break;
			}
			return pkt;
		},
	},

	certificateRequest: {
		name: "certificateRequest",
		msgType: certificate_request,
		rsa_sign: 1,
		dss_sign: 2,
		rsa_fixed_dh: 3,
		dss_fixed_dh: 4,

		unpacketize(session, s) {
			session.traceProtocol(this);
			var nCertTypes = s.readChar();
			var types = [];
			for (var i = 0; i < nCertTypes; i++) {
				var type = s.readChar();
				switch (type) {
				case this.rsa_sign:
					types.push(SSL.cipherSuite.CERT_RSA);
					break;
				case this.dss_sign:
					types.push(SSL.cipherSuite.CERT_DSA);
					break;
				default:
					// trace("SSL: certificateRequest: unsupported cert type: " + type + "\n");
					break;
				}
			}
			var ttlSize = s.readChars(2);
			var names = [];
			while (ttlSize > 0) {
				var nbytes = s.readChars(2);
				names.push(s.readChunk(nbytes));
				ttlSize -= nbytes + 2;
			}
			session.clientCerts = session.certificateManager.findPreferedCert(types, names);
			if (!session.clientCerts)
				session.clientCerts = [];	// proceed to the "certificate" protocol with a null certificate
		},
		packetize(session, types, authorities) {
			session.traceProtocol(this);
			var s = new SSLStream();
			s.writeChar(types.length);
			for (var i = 0; i < types.length; i++) {
				switch (types[i]) {
				case SSL.cipherSuite.CERT_RSA:
					s.writeChar(this.rsa_sign);
					break;
				case SSL.cipherSuite.CERT_DSA:
					s.writeChar(this.dss_sign);
					break;
				}
			}
			var ttlSize = 0;
			for (var i = 0; i < authorities.length; i++)
				ttlSize += authorities[i].length + 2;
			s.writeChars(ttlSize, 2);
			for (var i = 0; i < authorities.length; i++) {
				s.writeChars(authorities[i].length, 2);
				s.writeChunk(authorities[i]);
			}
			return handshakeProtocol.packetize(session, certificate_request, s);
		},
	},

	serverHelloDone: {
		name: "serverHelloDone",
		msgType: server_hello_done,
		unpacketize(session, s) {
			session.traceProtocol(this);
			// nothing to unpacketize
		},
		packetize(session) {
			session.traceProtocol(this);
			// nothing to packetize
			return handshakeProtocol.packetize(session, server_hello_done, new SSLStream());
		},
	},

	clientKeyExchange: {
		name: "clientKeyExchange",
		msgType: client_key_exchange,
		generateMasterSecret(session, preMasterSecret) {
			var PRF = require.weak("ssl/prf");
			var random = session.clientRandom;
			random = random.concat(session.serverRandom);
			session.masterSecret = PRF(session, preMasterSecret, master_secret_label, random, 48);
		},
		unpacketize(session, s) {
			session.traceProtocol(this);
			var n = s.readChars(2);
			var cipher = s.readChunk(n);
			let preMasterSecret;
			switch (session.chosenCipher.keyExchangeAlgorithm) {
			case SSL.cipherSuite.RSA:
				// PKCS1.5
				if (!session.myCert)
					throw new Error("SSL: clientKeyExchange: no cert");	// out of sequence
				var key = session.certificateManager.getKey(/* self */);
				var rsa = new Crypt.PKCS1_5(key, true);
				var plain = rsa.decrypt(cipher);
				// the first 2 bytes must be client_version
				var version = new Uint8Array(plain);
				if (((version[0] << 8) | version[1]) != session.protocolVersion) {
					session.alert = {level: 2, description: 70};
					return;
				}
				preMasterSecret = plain;
				break;
			case SSL.cipherSuite.DHE_DSS:
			case SSL.cipherSuite.DHE_RSA:
			case SSL.cipherSuite.DH_ANON:
			case SSL.cipherSuite.DH_DSS:
			case SSL.cipherSuite.DH_RSA:
				// implicit DH is not supported
				let Arith = require.weak("arith");
				let dh = session.dh;
				let y = new Arith.Integer(cipher);
				let mod = new Arith.Mont({z: new Arith.Z(), m: dh.p});
				y = mod.exp(y, dh.x);
				preMasterSecret = y.toChunk();
				break;
			default:
				throw new Error("SSL: clientKeyExchange: unsupported algorithm");
				break;
			}
			this.generateMasterSecret(session, preMasterSecret);
		},
		packetize(session) {
			session.traceProtocol(this);
			let s = new SSLStream();
			let preMasterSecret;
			switch (session.chosenCipher.keyExchangeAlgorithm) {
			case SSL.cipherSuite.RSA:
				var plain = new SSLStream();
				plain.writeChars(session.protocolVersion, 2);
				plain.writeChunk(Crypt.rng(46));
				preMasterSecret = plain.getChunk();
				var key = session.certificateManager.getKey(session.peerCert);
				var rsa = new Crypt.PKCS1_5(key);
				var cipher = rsa.encrypt(preMasterSecret);
				s.writeChars(cipher.byteLength, 2);
				s.writeChunk(cipher);
				break;
			case SSL.cipherSuite.DHE_DSS:
			case SSL.cipherSuite.DHE_RSA:
			case SSL.cipherSuite.DH_ANON:
			case SSL.cipherSuite.DH_DSS:
			case SSL.cipherSuite.DH_RSA:
				let Arith = require.weak("arith");
				// we don't support fixed key DH in cert so should send DH anyway
				if (!session.dhparams)
					throw new Error("SSL: clientKeyExchange: no DH params");
				let dh = session.dhparams;
				let r = Crypt.rng(dh.dh_p.byteLength);
				let x = new Arith.Integer(r);
				let g = new Arith.Integer(dh.dh_g);
				let p = new Arith.Integer(dh.dh_p);
				let mod = new Arith.Mont({z: new Arith.Z(), m: p});
				let y = mod.exp(g, x);
				let Yc = y.toChunk();
				s.writeChars(Yc.byteLength, 2);
				s.writeChunk(Yc);
				y = new Arith.Integer(dh.dh_Ys);
				y = mod.exp(y, x);
				preMasterSecret = y.toChunk();
				break;
			default:
				throw new Error("SSL: clientKeyExchange: unsupported algorithm");
				break;
			}
			this.generateMasterSecret(session, preMasterSecret);
			return handshakeProtocol.packetize(session, client_key_exchange, s);
		},
	},

	certificateVerify: {
		name: "certificateVerify",
		msgType: certificate_verify,
		calculateDigest(session) {
			return handshakeDigestResult(session,
						     session.protocolVersion >= 0x303 ? SHA256 :
						     (session.chosenCipher.keyExchangeAlgorithm == SSL.cipherSuite.RSA ? MD5 | SHA1 : SHA1));
		},
		unpacketize(session, s) {
			session.traceProtocol(this);
			var n = s.readChars(2);
			var sig = s.readChunk(n);
			if (session.chosenCipher.keyExchangeAlgorithm == SSL.cipherSuite.RSA) {
				var key = session.certificateManager.getKey(session.peerCert);
				var rsa = new Crypt.PKCS1_5(key);
				if (!rsa.verify(this.calculateDigest(session), sig))
					throw new Error("SSL: certificateVerify: auth err");
			}
			else
				throw new Error("SSL: certificateVerify: unimplemented");	// unimplemented
		},
		packetize(session, cert) {
			session.traceProtocol(this);
			if (session.chosenCipher.keyExchangeAlgorithm == SSL.cipherSuite.RSA) {
				if (!session.myCert)
					throw new Error("SSL: certificateVerify: no cert");	// out of sequence
				var key = session.certificateManager.getKey(cert);
				var rsa = new Crypt.PKCS1_5(key, true);
				var sig = rsa.sign(this.calculateDigest(session));
				var s = new SSLStream();
				s.writeChars(sig.byteLength, 2);
				s.writeChunk(sig);
			}
			else
				throw new Error("SSL: certificateVerify: unimplemented");	// unimplemented
			return handshakeProtocol.packetize(session, certificate_verify, s);
		},
	},

	finished: {
		name: "finished",
		msgType: finished,

		calculateVerifyData(session, flag) {
			var PRF = require.weak("ssl/prf");
			var finishLabel = (session.connectionEnd ^ flag) ? client_finished_label : server_finished_label;
			var digest = handshakeDigestResult(session, session.protocolVersion <= 0x302 ? MD5 | SHA1 : SHA256);
			return PRF(session, session.masterSecret, finishLabel, digest, 12);
		},
		unpacketize(session, s) {
			session.traceProtocol(this);
			var verify = this.calculateVerifyData(session, 1);
			if (Bin.comp(verify, s.readChunk(verify.byteLength)) != 0) {
				session.masterSecret = null;
				throw new Error("SSL: finished: auth err");
			}
		},
		packetize(session) {
			session.traceProtocol(this);
			var verify = this.calculateVerifyData(session, 0);
			return handshakeProtocol.packetize(session, finished, new SSLStream(verify));
		},
	},
};
export default handshakeProtocol;
