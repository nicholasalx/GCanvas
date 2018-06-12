const gulp = require('gulp');
const pug = require('pug');
const fs = require('fs-extra');
const less = require('gulp-less');
const yaml = require('yaml-js');
const marked = require('marked');
const watch = require('gulp-watch');
const highlight = require('gulp-highlight-code');


const homePageTasks = (function () {

    const templates = {

        index: pug.compile(
            fs.readFileSync('./template/index.pug', 'utf-8'),
            {
                filename: './template/index.pug',
                pretty: true
            }
        ),
        doc: pug.compile(
            fs.readFileSync('./template/doc.pug', 'utf-8'),
            {
                filename: './template/doc.pug',
                pretty: true
            }
        ),
        community: pug.compile(
            fs.readFileSync('./template/community.pug', 'utf-8'),
            {
                filename: './template/community.pug',
                pretty: true
            }
        ),
        playground: pug.compile(
            fs.readFileSync('./template/playground.pug', 'utf-8'),
            {
                filename: './template/playground.pug',
                pretty: true
            }
        )
    }

    function build() {

        const option = {
            root: './',
        }

        fs.writeFileSync('../website/index.html', templates.index(option));
        fs.writeFileSync('../website/community.html', templates.community(option));
        fs.writeFileSync('../website/playground.html', templates.playground({
            root: option.root,
            content: marked(fs.readFileSync('../GCanvas/docs/playground.md', 'utf-8'))
        }));

        const doc = yaml.load(fs.readFileSync('./index.yaml')).docs;

        function deal(docs, k, s) {
            if (typeof docs === 'string') {
                const content = fs.readFileSync(docs, 'utf-8');

                if (content) {
                    fs.outputFileSync(`../website/docs/${k}.html`, templates.doc(
                        {
                            index: doc,
                            content: marked(content),
                            root: '../'
                        }
                    ));
                } else {
                    throw new Error('Read source file failed, please run gulp fetch first.');
                }
            } else {
                for (let key in docs) {
                    deal(docs[key], key, docs);
                }
            }
        }

        deal(doc);

        return gulp.src('../website/docs/*.html')
            .pipe(highlight())
            .pipe(gulp.dest('../website/docs/'));
    }

    function lessTask() {
        return gulp.src('./style/index.less')
            .pipe(less())
            .pipe(gulp.dest('../website/'));
    }

    function watchLessTask() {
        return watch('./style/index.less')
            .pipe(less())
            .pipe(gulp.dest('../website/'));
    }

    function assets() {
        return gulp.src('./assets/**')
            .pipe(gulp.dest('../website/assets/'));
    }

    return { build, less: lessTask, watchLess: watchLessTask, assets };

})();

gulp.task('homepage-less', homePageTasks.less);
gulp.task('homepage-less-watch', homePageTasks.watchLess);
gulp.task('homepage-assets', homePageTasks.assets);
gulp.task('homepage-build', ['homepage-less', 'homepage-assets'], homePageTasks.build);


gulp.task('website', ['homepage-build'], function () { });