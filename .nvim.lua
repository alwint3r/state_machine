require('lspconfig').clangd.setup {
	cmd = {
		"clangd",
		"--compile-commands-dir=build",
		"--background-index",
		"--function-arg-placeholders=false",
	},
	filetypes = { "c", "cpp" },
	root_dir = require('lspconfig').util.root_pattern("CMakeLists.txt"),
	on_attach = function(client, bufnr)
		if client.server_capabilities.documentFormattingProvider then
			local grp = vim.api.nvim_create_augroup("LspFormat." .. bufnr, { clear = true })
			vim.api.nvim_create_autocmd("BufWritePre", {
				group = grp,
				buffer = bufnr,
				callback = function()
					vim.lsp.buf.format { async = false }
				end,
			})
		end

		local opts = { noremap = true, silent = true, buffer = bufnr }
		vim.keymap.set('n', 'gd', vim.lsp.buf.definition, opts)
		vim.keymap.set('n', 'K', vim.lsp.buf.hover, opts)
		vim.keymap.set('n', 'gi', vim.lsp.buf.implementation, opts)
		vim.keymap.set('n', '<leader>rn', vim.lsp.buf.rename, opts)
		vim.keymap.set('n', '<leader>ca', vim.lsp.buf.code_action, opts)
		vim.keymap.set('i', '(', function()
			vim.api.nvim_feedkeys('(', 'n', false)
			vim.defer_fn(function() vim.lsp.buf.signature_help() end, 0)
		end, opts)
	end,

	capabilities = require('cmp_nvim_lsp').default_capabilities(),
}

vim.keymap.set('n', '<leader>b', '<cmd>TermExec cmd="./build.sh"<cr>', { desc = 'Build project' })
vim.keymap.set('n', '<leader>r', '<cmd>TermExec cmd="./run.sh"<cr>', { desc = 'Run project' })
