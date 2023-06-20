#include "StarSha256.hpp"
#include "StarEncode.hpp"
#include "StarPythonic.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(ShaTest, All) {
  Sha256Hasher hasher;
  List<ByteArray> computed;
  List<ByteArray> expected;

  computed.append(sha256("/animations/muzzleflash/bulletmuzzle1/bulletmuzzle1.png"));
  expected.append(hexDecode("7e4a2cbe3826945d5f6593b347b49285b00ba8471cb4f6edc9b045c2cb3b6ea6"));
  computed.append(sha256(""));
  expected.append(hexDecode("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
  computed.append(sha256("The quick brown fox jumped over the lazy dog."));
  expected.append(hexDecode("68b1282b91de2c054c36629cb8dd447f12f096d3e3c587978dc2248444633483"));
  computed.append(sha256("The quick brown fox jumped over the lazy dog"));
  expected.append(hexDecode("7d38b5cd25a2baf85ad3bb5b9311383e671a8a142eb302b324d4a5fba8748c69"));
  computed.append(sha256("1"));
  expected.append(hexDecode("6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b"));
  computed.append(sha256("22"));
  expected.append(hexDecode("785f3ec7eb32f30b90cd0fcf3657d388b5ff4297f2f9716ff66e9b69c05ddd09"));
  computed.append(sha256("333"));
  expected.append(hexDecode("556d7dc3a115356350f1f9910b1af1ab0e312d4b3e4fc788d2da63668f36d017"));
  computed.append(sha256("four"));
  expected.append(hexDecode("04efaf080f5a3e74e1c29d1ca6a48569382cbbcd324e8d59d2b83ef21c039f00"));
  computed.append(sha256("five5"));
  expected.append(hexDecode("9caa1a017da4f2a1b836d0b4826549f651f3318bca4569af1d12549a92a3e0b0"));
  computed.append(sha256("sixsix"));
  expected.append(hexDecode("aec04bf8cf83291ceed3d47e8601e1989dc29ce76a6399264132de2731e69e55"));
  computed.append(sha256("seven77"));
  expected.append(hexDecode("363025b80362011c5f647eb99f029fc95f7df417a1aec169227a4fbb72f17f0f"));
  computed.append(sha256("eight888"));
  expected.append(hexDecode("ed13590c2bf3fc9df3a2f8631f117a70b67f6776480b9dd70125d1a4990e39f1"));
  computed.append(sha256("nine char"));
  expected.append(hexDecode("eb9d9e1b52a1e31daa806fbaf6db9b81ba1509cb933858af7ac73ba56e7d4c7f"));
  computed.append(sha256("1010101010"));
  expected.append(hexDecode("179b1a3e9e319cff0ca6671e07a62dad7d087b0f2a89eef0202612c3cb46baf9"));
  computed.append(sha256("eleven char"));
  expected.append(hexDecode("9734f6b41c8669fd1c6b5ad5ed8be0f733571933c7ec1bfe6061cc17944eeb48"));
  computed.append(sha256("tweleve char"));
  expected.append(hexDecode("219d8903ddaf708532901ac17f730b1f10a8a2aeb790ceb1e6883a8fa6960fc5"));
  computed.append(sha256("thirteen char"));
  expected.append(hexDecode("a7979658ac2830a4b39910c5eb8420b28dbf5a594210b102ff8faf76220c3895"));
  computed.append(sha256("fourteen chars"));
  expected.append(hexDecode("6099743911ee19e952125d8534ac0248bc1f83c87303ceef79e7c80e417c248a"));
  computed.append(sha256("15 char long :)"));
  expected.append(hexDecode("cc5b9b929c4e57a18c6e35191361a7d7eb825da773c13a006f71c4180acba379"));
  computed.append(sha256("16 chars long :)"));
  expected.append(hexDecode("5fb4f6a8d4a7f1918a94d8cf88e9efe893b7ce5dca92c6ff9c531eeb34192911"));
  computed.append(sha256("seventeenseventee"));
  expected.append(hexDecode("c86aa346db174a97bc41140be8778a17c305224a279c18c518c1213b5072d5ef"));
  computed.append(sha256("eighteeneighteen:)"));
  expected.append(hexDecode("686a19974520da4cde42bdfb930d63a147325c5461573c91adcd20aca3209b35"));
  computed.append(sha256("19 char winner here"));
  expected.append(hexDecode("f889777c4c1a74030e7db49e92f1d531d85638c04886c1605c90839b671a1f9e"));
  computed.append(sha256("this is 20 char long"));
  expected.append(hexDecode("95e85585cf3ebb0a8adb32f8a73663dc29bbe0e683bc44725af0f37b743f0afc"));
  computed.append(sha256("this is 21 chars long"));
  expected.append(hexDecode("64ce72b12e8e9dd32b5263d3ee69b3fb3122a040430b1626143a8ca14541cafc"));
  computed.append(sha256("22 chars right here..."));
  expected.append(hexDecode("a3799668feaef5fbaf693d6ae8202499397bd76ff1d0cf128f016bf9de1ab38c"));
  computed.append(sha256("23 right here right now"));
  expected.append(hexDecode("86ee2ce2e66d5a58a24a7dc2467d3f06abbceae0e047d8c62915c066c13de7e4"));
  computed.append(sha256("24 characters right here"));
  expected.append(hexDecode("b459283fd97ae84d988924773b366e813265adcc7c17cdbef0636f63ced73b22"));
  computed.append(sha256("count to 25 anyone? noone"));
  expected.append(hexDecode("abcbd8876f1e988a810c5a85db5643eeecd896f3cace9a5cd5e60af7ec8da858"));
  computed.append(sha256("26 chars in this string :)"));
  expected.append(hexDecode("86cbb112bd10aa23349bc49a9debeb91e9797da03937fd3d0ce4999ea3054bed"));
  computed.append(sha256("give me 27 characters or :("));
  expected.append(hexDecode("dab1030a321027fdc38df8c7110255c8be4b1477cc5d298de4e26d76b1af5697"));
  computed.append(sha256("this string is 28 characters"));
  expected.append(hexDecode("e073ef666b321eb5e8330ba852043896acc33d358a0efe1aac23417c1ad21662"));
  computed.append(sha256("29 character strings are best"));
  expected.append(hexDecode("56baa88cdbdc29ac09455f9a6bb318b3e5a6eed338fb54640ffa84e697a22e91"));
  computed.append(sha256("everyone loves 30 char strings"));
  expected.append(hexDecode("3459e7ebe0a2ef0be70d4097e4b63f7d7780abd02965faa0786a8b5bae49462e"));
  computed.append(sha256("31 char strings are cool though"));
  expected.append(hexDecode("062ce79b19e7cd44e0fe07a9183a123bcbb15740a31236c8bf14376df1ce4951"));
  computed.append(sha256("I prefer 32 character strings :)"));
  expected.append(hexDecode("937a2fda340bab7bc6789aebd467f77c22c3e970ba578882c390c77050253774"));
  computed.append(sha256("33 characters for the win? maybe!"));
  expected.append(hexDecode("4f7a34b15d5aae90309a9cc25e9f931b5de26834c3f6f69089561610802fc552"));
  computed.append(sha256("Running out of ideas 34 chars here"));
  expected.append(hexDecode("f5f4eca8faa433a4b98cc497e406f91e3b6e86d3024ca5a82e5f344db71e9966"));
  computed.append(sha256("What's 7 * 5? Did I hear 35 chars??"));
  expected.append(hexDecode("a914e66807993f19fea27b718c78ff28216b4851e79d3375c00e9103d6f7ee29"));
  computed.append(sha256("36 is a perfect square of a perf num"));
  expected.append(hexDecode("ebe580788c7c0c8f34f4e098ab4175b17603ee2f8a6a5e082bdb6e89c71ba977"));
  computed.append(sha256("This is getting a touch tedious... 37"));
  expected.append(hexDecode("2d596213f63a7a93237e59f692546a3c6f8b4e9f52a282bfb35828817372b3a2"));
  computed.append(sha256("Coming up with these isn't really hard"));
  expected.append(hexDecode("9462d6ca0d859423aa262e8e032635f16999cbd78c3d1aa7a7fd5a0247149e13"));
  computed.append(sha256("I mean, 39 chars isn't too bad to do..."));
  expected.append(hexDecode("d9166eba746094a585185a0a40ee7f684e5ffa7c1e3c30ee3b4d8a0cbfa2bfb0"));
  computed.append(sha256("You think it would be based on the limit"));
  expected.append(hexDecode("079874dc87365598b5feea42b9523d17fd75d791b07e187bbd7c9a4b335680dd"));
  computed.append(sha256("I mean, they just happen to come up right"));
  expected.append(hexDecode("6cc9f9a5f4e12aa57ef6ce736634f6ee93aac682e0d3fbb7a55574e06ece0e4f"));
  computed.append(sha256("I just can't explain that. Like the tides."));
  expected.append(hexDecode("21a55b18cc186e7c288bbba642d29be75bac0d08c58b2ec9d4956e11099ada66"));
  computed.append(sha256("I need 43 characters?  I get 43 characters."));
  expected.append(hexDecode("004bae6a27b46c06d64b1df29d2f53f3b409552ec031d961e8886a748b831d06"));
  computed.append(sha256("Simple as that. Don't know why, just is!! :)"));
  expected.append(hexDecode("66f03337b1b0d702904bebbfafc1bf6fbcb7e97e8284c3371eeb2d7538e89225"));
  computed.append(sha256("So I guess even though it's boring, it's fun."));
  expected.append(hexDecode("7a4036fafac9c46f0373471d50e3ff0ff05ea545060ca43919b726350eb32c45"));
  computed.append(sha256("I'll stop complaining about my problems though"));
  expected.append(hexDecode("0803b1553945c6158dee5e4b0a630e29c8c924403ebb03a8f4da34967d4c15ab"));
  computed.append(sha256("I sound so entitled, whaaa I'm typing words. :("));
  expected.append(hexDecode("b4df42295e968cd5ad71773fb9a813e8e4e66a41c4c53a16d29e746b879adaed"));
  computed.append(sha256("It's not really fair for people with real issues"));
  expected.append(hexDecode("dbed0bcca6b0dae74e78c5106de42936db7d2a4c318b04f233c9e48423e9e691"));
  computed.append(sha256("49 characters? How am I going to fill that up? :("));
  expected.append(hexDecode("9bffff6740825a4891584f5dad4d8725cd4918c80d4a9ab6471f6d6fec56ab51"));
  computed.append(sha256("That is not a real problem. Just me whining. Pish!"));
  expected.append(hexDecode("ce99e892a27c3e6a99f880003759bf16a7191f3acdbb7d542cd50013b1303ba4"));
  computed.append(sha256("So 51 characters long string huh?  Who knew? Not me"));
  expected.append(hexDecode("3fc667f8c419beba75f9ec99b342984bb1bda584c463b95dc9d0f006badd8f5e"));
  computed.append(sha256("And this one is 52. I guess it's alright. I guess..."));
  expected.append(hexDecode("6ee879d588e96ee70c71c98002913d3a8ecd21c07c57aea48a578e22e8366758"));
  computed.append(sha256("Someone order a 53 character long string? Anyone? No?"));
  expected.append(hexDecode("597a49f9a193db96804984b3ca4228f3e267f65f864ade5906949e14ff0f12b7"));
  computed.append(sha256("Well that's good! All I have is this 54 character one."));
  expected.append(hexDecode("ec6fde57b976adee10014c58f87403731b4b79f8442eeb606f9697c0deae9b11"));
  computed.append(sha256("This is the one we were having problems with: 55 chars."));
  expected.append(hexDecode("6bb7bf41f5f6e625d39e54d58fcc4e458a3af2e30d85eb90199de31fd668a16a"));
  computed.append(sha256("This is a 56 char string. Just making sure it's good too"));
  expected.append(hexDecode("446867368ec996d69a822e63454917372c0d0a23d46e6727730b975fcb7a8f25"));
  computed.append(sha256(
      "And this is a really long string that I'm testing to make sure that long strings do well in this function "
      "because who can know if the change I made fucked anything up!  Not me says I!  I have to test it first before I "
      "know if it actually works or not.  So I'm sitting here typing up an extremely long string just so I know if my "
      "changed royally cocked anything up beyond recognition.  So where are we at now?  Only about 4 or 5 blocks?  Not "
      "enough says I!  Keep going, ya bildge lickers.  Type your fingers off!  Bored now reading your mind.  Ha!  That "
      "thing was a girl.  Did you just read my mind?  Yup!  How?!  Muffin button."));
  expected.append(hexDecode("9253b707d18e956cde492ae999a67957a48a6720ad76a6cfa2046f697ebeafd3"));

  for (auto i : zip(computed, expected)) {
    EXPECT_TRUE(std::get<0>(i) == std::get<1>(i));
  }
}
