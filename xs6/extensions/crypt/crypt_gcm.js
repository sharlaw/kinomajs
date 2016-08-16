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
import Arith from "arith";
import Bin from "bin";

export default class GCM {
	constructor(cipher, tagLength = 16) {
		this.block = cipher;
		this.ctr = new Crypt.CTR(this.block);
		this.tagLength = tagLength;
	};
	init(iv, aad) {
		let h = this.block.encrypt(new ArrayBuffer(this.block.blockSize));
		this.ghash = new Crypt.GHASH(h, aad);
		if (iv.byteLength == 12) {
			let one = new DataView(new ArrayBuffer(4));
			one.setUint32(0, 1);	// big endian
			iv = iv.concat(one.buffer);
		}
		else {
			let ghash = new Crypt.GHASH(h);
			iv = ghash.process(iv);
		}
		this.y0 = iv;
		// start with y1
		let y1 = new Arith.Integer(iv);
		y1.inc();
		this.ctr.setIV(y1.toChunk());
	}
	encrypt(data, buf) {
		buf = this.ctr.encrypt(data, buf);
		this.ghash.update(buf);
		return buf;
	};
	decrypt(data, buf) {
		this.ghash.update(data);
		return this.ctr.decrypt(data, buf);
	};
	close() {
		let t = this.ghash.close();
		return Bin.xor(t, this.block.encrypt(this.y0));
	};
	process(data, buf, iv, aad, encFlag) {
		if (encFlag) {
			this.init(iv, aad);
			buf = this.encrypt(data, buf);
			let tag = this.close();
			if (tag.byteLength > this.tagLength)
				tag = tag.slice(0, this.tagLength);
			return buf.concat(tag);
		}
		else {
			this.init(iv, aad);
			buf = this.decrypt(data.slice(0, data.byteLength - this.tagLength), buf);
			let tag = this.close();
			if (Bin.comp(tag, data.slice(data.byteLength - this.tagLength), this.tagLength) == 0)
				return buf;
		}
	};
};
