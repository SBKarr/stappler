// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
 Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

#include "SPFilesystem.h"
#include "SPData.h"
#include "SPDataStream.h"
#include "SPLog.h"

#include "SPugLexer.h"
#include "SPugTemplate.h"
#include "SPugContext.h"

using namespace stappler;

const auto s_attrText = R"(
- var value = "Hello World!"
div(class='div-class' '(click)'='play()' + 'test')
input(
  type!='chec&kbox'
  name='agree&ment'
  value=value
  checked
)
|
|
a(href='google.com' value=value) Google
|
|
a(class='button' href='google.com') Google
|
|
a(class='button', href='google.com') Google
|
|
div(class='div-class', (click)='play()')
)";


const auto s_attrText2 = R"(
- var classes = ['foo', 'bar', 'baz']
a(class=classes)
|
|
a(style={color: 'red', background: 'green'})
|
|
input(type='checkbox' checked)
|
|
input(type='checkbox' checked=true)
|
|
input(type='checkbox' checked=false)
|
|
input(type='checkbox' checked=true && 'checked')
)";

const auto s_attrText3 = R"Test(

- var id = 12
a(href="/lecture/"+id @click.stop.prevent="toggleShowChapter(chapter)") link1

a(@click.stop.prevent="toggleShowChapter(chapter)" :href="getLectureLink(lecture)") link2

a(:href="getLectureLink(lecture)")
div#foo(data-bar="foo")&attributes({'data-foo': 'bar'})
-
  var b = {
    a: "A",
    b: "B",
    c: "C",
    d: "D",
    e: "E"
  }

#bar(data-bar="foo")&attributes(b)

)Test";

const auto s_outputText = R"(
html
	- var hello = "Hello"
	object= { hello: hello }
	hello= hello
	empty-obj= { }
	empty-arr= [ ]
	head
		title= hello + " " + world
	p= world
	object= { hello: hello, world: world, test: hello + " " + world }
	array= ["test", 1, false, hello]
	p= "<test>test & string</test>"
	p!= "<test>test & string</test>"
)";

const auto s_codeText = R"(
- for (var x = 0; x < 3; x++)
  li item
- var test = ["test", 1, false, hello]
-
  var list = ["Uno", "Dos", "Tres",
  "Cuatro", "Cinco", "Seis"]
head
  title= hello + " " + world
)";

const auto s_varText = R"(
html
	-
		var b = {
			a: "A",
			b: "B",
			c: "C",
			d: "D",
			e: "E"
		}

		var test = { data : { first: 1, second: 2 }, hello: "Hello", test: "Hello" + " " + "World", world: "World"}
		test.undef = "Undef"
		b.data = { first: 1, second: 2 }
	- var hello = ["test", 1, false]
	//p= hello
	p= test
	p= b
	p= test.data.first + " " + test.world + " " + test.undef
	p= { a: "A", b: "B" }.b
	p= hello[1]
	p= hello[0] + " " + hello[1] + " " + hello[2]
	p= test['data']
	p= [ ]
	p= { }
)";

const auto s_callText = R"(
- var testFn = "test"
  p test
p= testFn()
p= func()
p!= func(testFn, world, "a", "b")
)";

const auto s_tagText = R"(
ul
  li Item A
  li Item B
  li Item C
a: img
)";

const auto s_caseText = R"(
- var friends = 10
case friends
  when 0
    p you have no friends
  when 1
    p you have a friend
  default
    p you have #{friends} friends

case friends
  when 0
    p you have no friends
  when 10
  when 11
    p you have very few friends
  default
    p you have #{friends} friends

- friends = 1
case friends
  when 0: p you have no friends
  when 1: p you have a friend
  default: p you have #{friends} friends
)";

const auto s_ifText = R"(
//- var user = { description: 'foo bar baz' }
- var authorised = false
p.description test
#user
  if user.description
    h2.green Description
    p.description= user.description
  else if authorised
    h2.blue Description
    p.description.
      User has no description,
      why not add one...
  else
    h2.red Description
    p.description User has no description
  unless authorised
    p You're logged in as #{user.name}
  else
    p You're not logged in
)";

const auto s_eachText = R"(
p= {1:'one',2:'two',3:'three'}
ul
  each val in [1, 2, 3, 4, 5]
    li= val

ul
  each val, index in ['zero', 'one', 'two']
    li= index + ': ' + val

ul
  each val, index in {1:'one',2:'two',3:'three'}
    li= index + ': ' + val

- var values = [1, 2, 3, 4, 5];
ul
  each val in values.length ? values : ['There are no values']
    li= val

- values = [];
ul
  each val in values.length ? values : ['There are no values']
    li= val

ul
  each val in values
    li= val
  else
    li There are no values
)";


const auto s_condOpText = R"(
- var value1 = "value1"
- var value2 = "value2"
- true ? value1 : value2 = "test"
- var val = true ? value1 + " test" : value2
p= value1 + " " + value2
p= val
p= true ? false ? "truefalse" : "truetrue" : "false" + " test"
p= call(true ? "true" : "false", " test")
)";

const auto s_whileText = R"(
- var n = 0;
ul
  while n < 4
    li= n++
)";

const auto s_InterpText = R"(
- var title = "On Dogs: Man's Best Friend";
- var author = "enlore";
- var theGreat = "<span>escape!</span>";

h1= title
p Written with love by #{author}
p This will be safe: #{theGreat}
p No escaping for #{'}'}!
p Escaping works with \#{interpolation}
p Interpolation works with #{'#{interpolation}'} too!

- var riskyBusiness = "<em>Some of the girls are wearing my mother's clothing.</em>";
.quote
  p Joel: !{riskyBusiness}
)";


const auto s_tagInterpText = R"(
- var test = "test\ntest"
p= test
p.
  This is a very long and boring paragraph that spans multiple lines.
  Suddenly there is a #[strong strongly worded phrase] that cannot be
  #[em ignored].
p.
  And here's an example of an interpolated tag with an attribute:
  #[q(lang="es") ¡Hola Mundo!]

p
  | If I don't write the paragraph with tag interpolation, tags like
  strong strong
  | and
  em em
  | might produce unexpected results.
p.
  If I do, whitespace is #[strong respected] and #[em everybody] is happy.
)";

const auto s_tagMixinText = R"(
mixin article(value, title='Default Title')
  .article
    .article-wrapper
      h1= title
      p= value

+article("Value", "Header")
+article("Default Value")

//- Declaration
mixin list
  ul
    li foo
    li bar
    li baz
//- Use

mixin pet(name)
  li.pet= name
ul
  +pet('cat')
  +pet('dog')
  +pet('pig')

+list
+list
+pet('pig'))";

const auto s_tagMixinListText = R"(
mixin updateForm(value)
	span= value

mixin provider
	ul
		li: +updateForm("Test1")
		li: +updateForm("Test2")
	ul
		li: span Test1
		li: span Test2

+provider
)";

const auto s_tagIfTest = R"(
if novar && nover.cond
	.test Content
)";

const auto s_notTest = R"(
- var app = { value: "value", number: 42 }

if !app.extra
	- app.extra = "extra"
	.extra Not extra
.test= app
)";

const auto s_recursionTest = R"(
mixin recursion(id)
	span= id
	span= value.value
	if value.next
		- var next = value.next
		+recursion(next)

+recursion(test)
)";

int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'v') {
		ret.setBool(true, "verbose");
	}
	return 1;
}

int parseOptionString(data::Value &ret, const String &str, int argc, const char * argv[]) {
	if (str == "v") {
		ret.setBool(true, "verbose");
	}
	return 1;
}

int _spMain(argc, argv) {
	data::Value opts = data::parseCommandLineOptions(argc, argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.getBool("verbose")) {
		std::cout << " Current work dir: " << stappler::filesystem::currentDir() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPath() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPath() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePath() << "\n";
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << opts << "\n";
	};

	memory::MemPool pool(memory::MemPool::Managed);
	memory::pool::push(pool);

	//auto &args = opts.getValue("args");

	pug::Template * tpl = pug::Template::read(s_tagMixinText, pug::Template::Options::getPretty());

	using Value = pug::Value;

	pug::Context ctx;

	tpl->describe(std::cout, true);

	ctx.set("test", Value{
		pair("value", Value("value1")),
		pair("next", Value{
			pair("value", Value("value2")),
			pair("next", Value{
				pair("value", Value("value3")),
			}),
		}),
	});
	ctx.set("world", Value("World"));
	ctx.set("func", [] (pug::VarStorage &self, pug::Var *args, size_t count) -> pug::Var {
		if (count == 0) {
			return pug::Var(Value("No arguments"));
		} else {
			memory::PoolInterface::StringStreamType stream;
			for (size_t i = 0; i < count; ++ i) {
				stream << " " << args[i].readValue();
			}
			return pug::Var(Value(stream.str()));
		}
		return pug::Var();
	});

	tpl->run(ctx, std::cout);

	memory::pool::pop();

	return 0;
}
