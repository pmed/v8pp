var console = require('console')

console.log('yo', 'baby', 4.2, null)

var c2 = require('console')
c2.log('conker 2')

function pando() {
    this.friend = 1
    this.masto = require('console')
    this.whack = function() {
        this.masto.log('truckman')
    }
}

var c = new pando()
c.whack()

